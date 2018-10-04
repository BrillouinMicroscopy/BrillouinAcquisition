#include "stdafx.h"
#include "Fluorescence.h"

Fluorescence::Fluorescence(QObject *parent, Acquisition *acquisition, PointGrey **pointGrey, NIDAQ **nidaq)
	: AcquisitionMode(parent, acquisition), m_pointGrey(pointGrey), m_NIDAQ(nidaq) {}

Fluorescence::~Fluorescence() {
}

void Fluorescence::initialize() {
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setGain(int gain) {
	m_settings.gain = gain;
}

ChannelSettings * Fluorescence::getChannelSettings(FLUORESCENCE_MODE mode) {
	if (mode == FLUORESCENCE_MODE::BLUE) {
		return &m_settings.blue;
	} else if (mode == FLUORESCENCE_MODE::GREEN) {
		return &m_settings.green;
	} else if (mode == FLUORESCENCE_MODE::RED) {
		return &m_settings.red;
	} else if (mode == FLUORESCENCE_MODE::BRIGHTFIELD) {
		return &m_settings.brightfield;
	} else {
		return nullptr;
	}
}

void Fluorescence::setChannel(FLUORESCENCE_MODE mode, bool enabled) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->enabled = enabled;
}

void Fluorescence::setExposure(FLUORESCENCE_MODE mode, int exposure) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->exposure = exposure;
}

void Fluorescence::startRepetitions() {
	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::FLUORESCENCE);
	if (!allowed) {
		return;
	}

	// reset abort flag
	m_abort = false;

	// configure camera for measurement
	m_settings.camera.roi.left = 128;
	m_settings.camera.roi.top = 0;
	m_settings.camera.roi.width = 1024;
	m_settings.camera.roi.height = 1024;
	m_settings.camera.readout.pixelEncoding = L"Raw8";
	m_settings.camera.readout.triggerMode = L"Internal";
	m_settings.camera.readout.cycleMode = L"Fixed";
	m_settings.camera.frameCount = 1;

	// read back the applied settings
	m_settings.camera = (*m_pointGrey)->getSettings();

	m_acquisition->newRepetition(ACQUISITION_MODE::FLUORESCENCE);

	// start repetition
	acquire(m_acquisition->m_storage);

	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);
}

void Fluorescence::acquire(std::unique_ptr <StorageWrapper> & storage) {
	m_status = ACQUISITION_STATUS::STARTED;
	emit(s_acquisitionStatus(m_status));

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_settings.camera.roi.height, m_settings.camera.roi.width };
	int bytesPerFrame = m_settings.camera.roi.width * m_settings.camera.roi.height;
	// Loop through the different modes
	int imageNumber{ 0 };
	for (gsl::index i{ 0 }; i < (int)FLUORESCENCE_MODE::MODE_COUNT; i++) {
		// Abort if requested
		if (m_abort) {
			this->abortMode();
			return;
		}

		ChannelSettings *channel = getChannelSettings((FLUORESCENCE_MODE)i);
		// Don't acquire this mode if it is not enabled
		if (!channel->enabled) {
			continue;
		}

		// Find correct preset
		ScanControl::SCAN_PRESET preset;
		std::string channelName;
		switch ((FLUORESCENCE_MODE)i) {
			case FLUORESCENCE_MODE::BLUE:
				preset = ScanControl::SCAN_EPIFLUOBLUE;
				channelName = "Blue";
				break;
			case FLUORESCENCE_MODE::GREEN:
				preset = ScanControl::SCAN_EPIFLUOGREEN;
				channelName = "Green";
				break;
			case FLUORESCENCE_MODE::RED:
				preset = ScanControl::SCAN_EPIFLUORED;
				channelName = "Red";
				break;
			case FLUORESCENCE_MODE::BRIGHTFIELD:
				preset = ScanControl::SCAN_EPIFLUOOFF;
				channelName = "Brightfield";
				break;
			default:
				return;
		}
		m_settings.camera.exposureTime = 1e-3*channel->exposure;
		(*m_pointGrey)->startAcquisition(m_settings.camera);

		// move to Fluorescence configuration
		(*m_NIDAQ)->setElements(preset);

		// read images from camera
		std::vector<unsigned char> images(bytesPerFrame);

			// acquire images
		int64_t pointerPos = 0 * (int64_t)bytesPerFrame;
		(*m_pointGrey)->getImageForAcquisition(&images[pointerPos]);

		// cast the vector to unsigned short
		std::vector<unsigned char> *images_ = (std::vector<unsigned char> *) &images;

		// store images
		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		FLUOIMAGE *img = new FLUOIMAGE(imageNumber, rank_data, dims_data, date, channelName, *images_);

		QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(FLUOIMAGE*, img));

		// configure camera for preview
		(*m_pointGrey)->stopAcquisition();
		imageNumber++;
	}

	m_status = ACQUISITION_STATUS::FINISHED;
	emit(s_acquisitionStatus(m_status));
}

void Fluorescence::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);
	m_status = ACQUISITION_STATUS::ABORTED;
	emit(s_acquisitionStatus(m_status));
}
