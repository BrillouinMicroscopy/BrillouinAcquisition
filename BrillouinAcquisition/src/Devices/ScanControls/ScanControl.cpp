#include "stdafx.h"
#include "ScanControl.h"

/*
 * Public definitions
 */

bool ScanControl::getConnectionStatus() {
	return m_isConnected && m_isCompatible;
}

void ScanControl::movePosition(POINT2 distance) {
	auto position = getPosition();
	auto newPosition = POINT2{ position.x, position.y } + distance;
	setPosition(newPosition);
}

void ScanControl::movePosition(const POINT3& distance) {
	auto position = getPosition() + distance;
	setPosition(position);
}

POINT3 ScanControl::getPosition(PositionType positionType) {
	auto pos = POINT2{};
	switch (positionType) {
		case PositionType::BOTH:
			// We return the absolute position, including the position of the stage and the scanner
			pos = m_positionStage + m_positionScanner;
			break;
		case PositionType::STAGE:
			pos = m_positionStage;
			break;
		case PositionType::SCANNER:
			pos = m_positionScanner;
			break;
		default:
			break;
	}

	return POINT3{ pos.x, pos.y, m_positionFocus };
}

/*
 * Public slots
 */

void ScanControl::setPositionRelativeX(double positionX) {
	// We use the base implementation of getPosition, so that
	// the hardware is not queried.
	auto position = ScanControl::getPosition();
	position.x = m_homePosition.x + positionX;

	setPosition(position);
}

void ScanControl::setPositionRelativeY(double positionY) {
	// We use the base implementation of getPosition, so that
	// the hardware is not queried.
	auto position = ScanControl::getPosition();
	position.y = m_homePosition.y + positionY;

	setPosition(position);
}

void ScanControl::setPositionRelativeZ(double positionZ) {
	// We use the base implementation of getPosition, so that
	// the hardware is not queried.
	auto position = ScanControl::getPosition();
	position.z = m_homePosition.z + positionZ;

	setPosition(position);
}

void ScanControl::locatePositionScanner(POINT2 positionLaserPix) {
	// Don't allow to locate the laser position manually if the scanControl supports capability LaserScanner
	if (supportsCapability(Capabilities::LaserScanner)) {
		return;
	}

	m_positionScanner = pixToMicroMeter(positionLaserPix);

	announcePositionScanner();
	announcePositions();
}

bool ScanControl::supportsCapability(Capabilities capability) {
	return std::find(m_capabilities.begin(), m_capabilities.end(), capability) != m_capabilities.end();
}

void ScanControl::setPositionInPix(POINT2 positionPix) {
	// This is the wanted position of the laser focus
	auto positionMicrometer = pixToMicroMeter(positionPix);
	// We have to subtract the position of the scanner to get to the relative movement
	positionMicrometer -= m_positionScanner;
	movePosition(positionMicrometer);
}

void ScanControl::enableMeasurementMode(bool enabled) {
	// When enabling the measurement mode, we have to safe the start position,
	// so the AOI positions display has the correct origin.
	if (enabled) {
		auto pos = getPosition();
		m_startPosition = POINT2{ pos.x, pos.y };
	}
	m_measurementMode = enabled;
}

void ScanControl::setPreset(ScanPreset presetType) {
	auto preset = getPreset(presetType);
	getElements();

	for (gsl::index ii{ 0 }; ii < m_deviceElements.size(); ii++) {
		// check if element position needs to be changed
		if (!preset.elementPositions[ii].empty() && !simplemath::contains(preset.elementPositions[ii], m_elementPositions[ii])) {
			setElement(m_deviceElements[ii], preset.elementPositions[ii][0]);
			m_elementPositions[ii] = preset.elementPositions[ii][0];
		}
	}
	checkPresets();
	emit(elementPositionsChanged(m_elementPositions));

	setPresetAfter(presetType);
}

Preset ScanControl::getPreset(ScanPreset presetType) {
	for (gsl::index ii{ 0 }; ii < m_presets.size(); ii++) {
		if (m_presets[ii].index == presetType) {
			return m_presets[ii];
		}
	}
	return m_presets[0];
}

void ScanControl::checkPresets() {
	// checks all presets if they are currenty active
	for (gsl::index ii{ 0 }; ii < m_presets.size(); ii++) {
		auto preset = m_presets[ii];
		auto active{ true };
		// check if an element position does not match the valid positions of a preset
		for (gsl::index jj{ 0 }; jj < preset.elementPositions.size(); jj++) {
			if (!preset.elementPositions[jj].empty() && !simplemath::contains(preset.elementPositions[jj], m_elementPositions[jj])) {
				m_activePresets &= ~preset.index;
				active = false;
				break;
			}
		}
		// set the preset active
		if (active) {
			m_activePresets |= preset.index;
		}
	}
}

bool ScanControl::isPresetActive(ScanPreset presetType) {
	return ScanPreset::SCAN_NULL != (presetType & m_activePresets);
}

void ScanControl::announcePosition() {
	auto point = getPosition();
	emit(currentPosition(point - m_homePosition));
}

void ScanControl::startAnnouncing() {
	startAnnouncingPosition();
	startAnnouncingElementPosition();
}

void ScanControl::stopAnnouncing() {
	stopAnnouncingPosition();
	stopAnnouncingElementPosition();
}

void ScanControl::startAnnouncingPosition() {
	if (m_positionTimer) {
		m_positionTimer->start(100);
	}
}

void ScanControl::stopAnnouncingPosition() {
	if (m_positionTimer) {
		m_positionTimer->stop();
	}
}

void ScanControl::startAnnouncingElementPosition() {
	if (m_elementPositionTimer) {
		m_elementPositionTimer->start(100);
	}
}

void ScanControl::stopAnnouncingElementPosition() {
	if (m_elementPositionTimer) {
		m_elementPositionTimer->stop();
	}
}

void ScanControl::setHome() {
	m_homePosition = getPosition();
	announceSavedPositionsNormalized();
	announcePosition();
	calculateHomePositionBounds();
}

void ScanControl::moveHome() {
	setPosition(m_homePosition);
}

void ScanControl::savePosition() {
	auto position = getPosition();
	m_savedPositions.push_back(position);
	announceSavedPositionsNormalized();
}

void ScanControl::moveToSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		setPosition(m_savedPositions[index]);
	}
}

void ScanControl::deleteSavedPosition(int index) {
	if (m_savedPositions.size() > index) {
		m_savedPositions.erase(m_savedPositions.begin() + index);
		announceSavedPositionsNormalized();
	}
}

std::vector<POINT3> ScanControl::getSavedPositionsNormalized() {
	auto savedPositionsNormalized = m_savedPositions;
	std::transform(savedPositionsNormalized.begin(), savedPositionsNormalized.end(), savedPositionsNormalized.begin(),
		[this](POINT3 point) {
			return point - this->m_homePosition;
		}
	);
	return savedPositionsNormalized;
}

void ScanControl::announceSavedPositionsNormalized() {
	auto savedPositionsNormalized = getSavedPositionsNormalized();
	emit(savedPositionsChanged(savedPositionsNormalized));
}

void ScanControl::setScaleCalibration(const ScaleCalibrationData& scaleCalibration) {
	/*
	 * In order to prevent having to relocate the scanner position,
	 * we convert the scanner position to pixel using the old scale calibration
	 * and back to micro meter using the new scale.
	 */
	auto posScanner = microMeterToPix(m_positionScanner);
	m_scaleCalibration = scaleCalibration;
	m_positionScanner = pixToMicroMeter(posScanner);

	calculateBounds();
	calculateHomePositionBounds();
	emit(s_scaleCalibrationChanged(convertPositionsToPix()));
}

ScaleCalibrationData ScanControl::getScaleCalibration() {
	return m_scaleCalibration;
}

std::vector<POINT2> ScanControl::getPositionsPix(const std::vector<POINT3>& positionsMicrometer) {
	// Cache the requested positions so we can re-emit updated positions
	// in case the scale calibration changes
	m_AOI_positions = positionsMicrometer;

	return convertPositionsToPix();
};

/*
 * Function converts a position in pixel to a position im µm.
 * This is relative to the origin (pixOrigin) and not on an absolute scale e.g. of the translation stage.
 */
POINT2 ScanControl::pixToMicroMeter(POINT2 positionPix) {
	positionPix -= m_scaleCalibration.originPix;
	return positionPix.x * m_scaleCalibration.pixToMicrometerX + positionPix.y * m_scaleCalibration.pixToMicrometerY;
}

POINT2 ScanControl::microMeterToPix(POINT2 positionMicrometer) {
	return (positionMicrometer.x * m_scaleCalibration.micrometerToPixX + positionMicrometer.y * m_scaleCalibration.micrometerToPixY)
		+ m_scaleCalibration.originPix;
}

POINT2 ScanControl::microMeterToPix(POINT3 positionMicrometer) {
	return microMeterToPix(POINT2{ positionMicrometer.x, positionMicrometer.y });
}

/*
 * Protected definitions
 */
void ScanControl::setPresetAfter(ScanPreset presetType) {}

void ScanControl::calculateBounds() {
	// Bounds of the stage
	m_absoluteBounds = {
		-150000,	// [µm] minimal x-value
		 150000,	// [µm] maximal x-value
		-150000,	// [µm] minimal y-value
		 150000,	// [µm] maximal y-value
		-150000,	// [µm] minimal z-value
		 150000		// [µm] maximal z-value
	};
}

void ScanControl::calculateHomePositionBounds() {
	m_homePositionBounds.xMin = m_absoluteBounds.xMin - m_homePosition.x;
	m_homePositionBounds.xMax = m_absoluteBounds.xMax - m_homePosition.x;
	m_homePositionBounds.yMin = m_absoluteBounds.yMin - m_homePosition.y;
	m_homePositionBounds.yMax = m_absoluteBounds.yMax - m_homePosition.y;
	m_homePositionBounds.zMin = m_absoluteBounds.zMin - m_homePosition.z;
	m_homePositionBounds.zMax = m_absoluteBounds.zMax - m_homePosition.z;

	emit(homePositionBoundsChanged(m_homePositionBounds));
}

void ScanControl::calculateCurrentPositionBounds() {
	auto currentPosition = getPosition();
	calculateCurrentPositionBounds(currentPosition);
}

void ScanControl::calculateCurrentPositionBounds(POINT3 currentPosition) {
	m_currentPositionBounds.xMin = m_absoluteBounds.xMin - currentPosition.x;
	m_currentPositionBounds.xMax = m_absoluteBounds.xMax - currentPosition.x;
	m_currentPositionBounds.yMin = m_absoluteBounds.yMin - currentPosition.y;
	m_currentPositionBounds.yMax = m_absoluteBounds.yMax - currentPosition.y;
	m_currentPositionBounds.zMin = m_absoluteBounds.zMin - currentPosition.z;
	m_currentPositionBounds.zMax = m_absoluteBounds.zMax - currentPosition.z;

	emit(currentPositionBoundsChanged(m_currentPositionBounds));
}

/*
 * This functions announces the updated AOI positions if necessary.
 * We check if the stage or scanner position has changed since the last announcement
 * and announce new positions under these conditions:
 *	- if the stage position has changed and a scan is currently running
 *	- if the scanner position has changed and no scan is running
 * (the AOI positions are static with respect to the laser focus during preview, and
 * static with respect to the sample during scanning).
 */
void ScanControl::announcePositions() {
	// Measurement mode and stage position didn't change significantly --> do nothing
	if (m_measurementMode && abs(m_positionStageOld - m_positionStage) < 1e-6) return;
	// Preview mode and scanner position didn't change significantly --> do nothing
	if (!m_measurementMode && abs(m_positionScannerOld - m_positionScanner) < 1e-6) return;

	// Set new positions if they have significantly changed
	m_positionScannerOld = m_positionScanner;
	m_positionStageOld = m_positionStage;

	emit(s_scaleCalibrationChanged(convertPositionsToPix()));
}

void ScanControl::announcePositionScanner() {
	auto positionScannerPix = microMeterToPix(m_positionScanner);
	emit(s_positionScannerChanged(positionScannerPix));
}

void ScanControl::registerCapability(Capabilities capability) {
	// Don't add a capability twice
	if (!supportsCapability(capability)) {
		m_capabilities.push_back(capability);
	}
}

/*
 * Private definitions
 */

std::vector<POINT2> ScanControl::convertPositionsToPix() {
	auto positionsPix = std::vector<POINT2>(m_AOI_positions.size());

	// In normal mode, the positions are shown relative to the scanner position.
	auto offset = m_positionScanner;
	// In measurement mode, the positions are shown relative to the start position.
	if (this->m_measurementMode) {
		offset = this->m_startPosition - m_positionStage;
	}

	std::transform(m_AOI_positions.begin(), m_AOI_positions.end(), positionsPix.begin(),
		[this, offset](POINT3 point) {
			return this->microMeterToPix(POINT2{ point.x, point.y } + offset);
		}
	);
	return positionsPix;
}