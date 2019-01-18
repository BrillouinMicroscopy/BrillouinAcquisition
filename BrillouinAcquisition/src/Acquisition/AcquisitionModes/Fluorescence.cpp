#include "stdafx.h"
#include "Fluorescence.h"

Fluorescence::Fluorescence(QObject* parent, Acquisition* acquisition, Camera** camera, ScanControl** scanControl)
	: AcquisitionMode(parent, acquisition), m_camera(camera), m_scanControl(scanControl) {}

Fluorescence::~Fluorescence() {
}

void Fluorescence::initialize() {
	m_settings.camera.gain = 10;
	emit(s_acqSettingsChanged(m_settings));
}

ChannelSettings* Fluorescence::getChannelSettings(FLUORESCENCE_MODE mode) {
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

std::vector<ChannelSettings*> Fluorescence::getEnabledChannels() {
	std::vector<ChannelSettings*> enabledChannels;
	for (gsl::index i{ 0 }; i < (int)FLUORESCENCE_MODE::MODE_COUNT; i++) {

		ChannelSettings* channel = getChannelSettings((FLUORESCENCE_MODE)i);

		if (channel == nullptr) {
			continue;
		}
		// Don't acquire this mode if it is not enabled
		if (!channel->enabled) {
			continue;
		} else {
			enabledChannels.push_back(channel);
		}
	}
	return enabledChannels;
}

void Fluorescence::configureCamera() {

	std::string cameraType = typeid(**m_camera).name();

	// configure camera for measurement
	// This needs a proper implementation with user defined values. Probably by a configuration file.
	m_settings.camera.roi.left = 128;
	m_settings.camera.roi.top = 0;
	m_settings.camera.roi.width = 1024;
	m_settings.camera.roi.height = 1024;
	if (cameraType == "class uEyeCam") {
		m_settings.camera.roi.left = 800;
		m_settings.camera.roi.top = 400;
		m_settings.camera.roi.width = 1800;
		m_settings.camera.roi.height = 2000;
		m_settings.camera.readout.triggerMode = L"Internal";
	}
	else if (cameraType == "class PointGrey") {
		m_settings.camera.roi.left = 128;
		m_settings.camera.roi.top = 0;
		m_settings.camera.roi.width = 1024;
		m_settings.camera.roi.height = 1024;
		m_settings.camera.readout.triggerMode = L"Software";
	}
	m_settings.camera.readout.pixelEncoding = L"Raw8";
	m_settings.camera.readout.cycleMode = L"Fixed";
	m_settings.camera.frameCount = 1;
}

void Fluorescence::setChannel(FLUORESCENCE_MODE mode, bool enabled) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->enabled = enabled;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setExposure(FLUORESCENCE_MODE mode, int exposure) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->exposure = exposure;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setGain(FLUORESCENCE_MODE mode, int gain) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->gain = gain;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::startStopPreview(FLUORESCENCE_MODE mode) {
	if (mode == previewChannel || mode == FLUORESCENCE_MODE::NONE) {
		// stop preview
		previewChannel = FLUORESCENCE_MODE::NONE;
		QMetaObject::invokeMethod((*m_camera), "stopPreview", Qt::AutoConnection);
	} else {
		// if preview is already running, stop it first
		if ((*m_camera)->m_isPreviewRunning) {
			(*m_camera)->stopPreview();
		}
		previewChannel = mode;
		ChannelSettings* channel = getChannelSettings(mode);

		// move to Fluorescence configuration
		(*m_scanControl)->setPreset(channel->preset);

		// start image acquisition
		m_settings.camera.exposureTime = 1e-3*channel->exposure;
		m_settings.camera.gain = channel->gain;
		(*m_camera)->setSettings(m_settings.camera);
		QMetaObject::invokeMethod((*m_camera), "startPreview", Qt::AutoConnection);
	}
	emit(s_previewRunning(previewChannel));
}

void Fluorescence::startRepetitions() {
	startRepetitions({});
}

void Fluorescence::startRepetitions(std::vector<FLUORESCENCE_MODE> modes) {
	std::vector<ChannelSettings *> channels;
	// If the provided mode vector is empty, acquire the enabled channels.
	if (!modes.size()) {
		channels = getEnabledChannels();
		// If no channels are enabled, return.
		if (!channels.size()) {
			return;
		}
	// Else, find the channel settings for the given modes.
	} else {
		for (auto const& mode : modes) {
			auto channel = getChannelSettings(mode);
			channels.push_back(channel);
		}
	}

	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::FLUORESCENCE);
	if (!allowed) {
		return;
	}

	// stop preview if necessary
	if (previewChannel != FLUORESCENCE_MODE::NONE) {
		previewChannel = FLUORESCENCE_MODE::NONE;
		(*m_camera)->stopPreview();
	}

	// reset abort flag
	m_abort = false;

	configureCamera();

	m_acquisition->newRepetition(ACQUISITION_MODE::FLUORESCENCE);

	// start repetition
	acquire(m_acquisition->m_storage, channels);

	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);
}

void Fluorescence::acquire(std::unique_ptr <StorageWrapper>& storage) {
	acquire(storage, {});
}

void Fluorescence::acquire(std::unique_ptr <StorageWrapper>& storage, std::vector<ChannelSettings *> channels) {
	m_status = ACQUISITION_STATUS::STARTED;
	emit(s_acquisitionStatus(m_status));

	// If the provided channel settings vector is empty, acquire the enabled channels.
	if (!channels.size()) {
		channels = getEnabledChannels();
		// If no channels are enabled, return.
		if (!channels.size()) {
			return;
		}
	}

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_settings.camera.roi.height, m_settings.camera.roi.width };
	int bytesPerFrame = m_settings.camera.roi.width * m_settings.camera.roi.height;
	// Loop through the different modes
	int imageNumber{ 0 };
	for (auto const& channel : channels) {
		// Abort if requested
		if (m_abort) {
			this->abortMode();
			return;
		}

		// move to Fluorescence configuration
		(*m_scanControl)->setPreset(channel->preset);

		/*
		 * We have to check if the exposure time or gain settings will change.
		 * If they will, then we have to trash the first image after the change,
		 * as the settings might not be applied already (valid for trigger mode).
		 * Might be worse for free running mode.
		 * See https://www.ptgrey.com/KB/10086
		 */
		bool changed{ false };
		if (((int)1e3*m_settings.camera.exposureTime != channel->exposure) || (m_settings.camera.gain != channel->gain)) {
			changed = true;
		}

		m_settings.camera.exposureTime = 1e-3*channel->exposure;
		m_settings.camera.gain = channel->gain;

		if (changed) {
			m_settings.camera.frameCount = 2;
			(*m_camera)->startAcquisition(m_settings.camera);
			(*m_camera)->getImageForAcquisition(nullptr, false);
			(*m_camera)->getImageForAcquisition(nullptr, false);
			(*m_camera)->stopAcquisition();
		}

		// start image acquisition
		m_settings.camera.frameCount = 1;
		(*m_camera)->startAcquisition(m_settings.camera);

		// read images from camera
		std::vector<unsigned char> images(bytesPerFrame);

			// acquire images
		int64_t pointerPos = 0 * (int64_t)bytesPerFrame;
		(*m_camera)->getImageForAcquisition(&images[pointerPos], true);

		// cast the vector to unsigned short
		std::vector<unsigned char>* images_ = (std::vector<unsigned char> *) &images;

		// store images
		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		FLUOIMAGE* img = new FLUOIMAGE(imageNumber, rank_data, dims_data, date, channel->name, *images_);

		QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(FLUOIMAGE*, img));

		// configure camera for preview
		(*m_camera)->stopAcquisition();
		imageNumber++;
		double percentage = 100 * (double)imageNumber / channels.size();
		int remaining = 1e-3 * measurementTimer.elapsed() / imageNumber * ((int64_t)channels.size() - imageNumber);
		emit(s_repetitionProgress(percentage, remaining));
	}

	m_status = ACQUISITION_STATUS::FINISHED;
	emit(s_acquisitionStatus(m_status));
}

void Fluorescence::abortMode() {
	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);
	m_status = ACQUISITION_STATUS::ABORTED;
	emit(s_acquisitionStatus(m_status));
}
