#include "stdafx.h"
#include "Fluorescence.h"

/*
 * Public definitions
 */

Fluorescence::Fluorescence(QObject* parent, Acquisition* acquisition, Camera** camera, ScanControl** scanControl)
	: AcquisitionMode(parent, acquisition, scanControl), m_camera(camera) {}

Fluorescence::~Fluorescence() {
}

/*
 * Public slots
 */

void Fluorescence::startRepetitions() {
	startRepetitions({});
}

void Fluorescence::startRepetitions(std::vector<FLUORESCENCE_MODE> modes) {
	std::vector<ChannelSettings*> channels;
	// If the provided mode vector is empty, acquire the enabled channels.
	if (!modes.size()) {
		channels = getEnabledChannels();
		// If no channels are enabled, return.
		if (!channels.size()) {
			return;
		}
		// Else, find the channel settings for the given modes.
	}
	else {
		for (auto const& mode : modes) {
			auto channel = getChannelSettings(mode);
			channels.push_back(channel);
		}
	}

	bool allowed = m_acquisition->enableMode(ACQUISITION_MODE::FLUORESCENCE);
	if (!allowed) {
		return;
	}

	// Save current preview channel to re-enable it after acquisition
	m_previousPreviewChannel = m_currentPreviewChannel;
	// stop preview if necessary
	if (m_currentPreviewChannel != FLUORESCENCE_MODE::NONE) {
		m_currentPreviewChannel = FLUORESCENCE_MODE::NONE;
		if (m_camera) {
			(*m_camera)->stopPreview();
		}
	}

	// reset abort flag
	m_abort = false;

	configureCamera();

	m_acquisition->newRepetition(ACQUISITION_MODE::FLUORESCENCE);

	// start repetition
	acquire(m_acquisition->m_storage, channels);

	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);

	if (m_previousPreviewChannel != FLUORESCENCE_MODE::NONE) {
		startStopPreview(m_previousPreviewChannel);
	}
}

void Fluorescence::initialize() {
	m_settings.camera.gain = 10;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setChannel(FLUORESCENCE_MODE mode, bool enabled) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->enabled = enabled;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setExposure(FLUORESCENCE_MODE mode, int exposure) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->exposure = exposure;
	// Apply the settings immediately if the mode preview is running
	if (m_currentPreviewChannel == mode) {
		m_settings.camera.exposureTime = 1e-3 * channel->exposure;
		if (m_camera) {
			QMetaObject::invokeMethod(
				(*m_camera),
				[&m_camera = (*m_camera), channel]() { m_camera->setSetting(CAMERA_SETTING::EXPOSURE, 1e-3 * channel->exposure); },
				Qt::AutoConnection
			);
		}
	}
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setGain(FLUORESCENCE_MODE mode, int gain) {
	ChannelSettings* channel = getChannelSettings(mode);
	channel->gain = gain;
	// Apply the settings immediately if the mode preview is running
	if (m_currentPreviewChannel == mode) {
		m_settings.camera.gain = channel->gain;
		if (m_camera) {
			QMetaObject::invokeMethod(
				(*m_camera),
				[&m_camera = (*m_camera), channel]() { m_camera->setSetting(CAMERA_SETTING::GAIN, channel->gain); },
				Qt::AutoConnection
			);
		}
	}
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::startStopPreview(FLUORESCENCE_MODE mode) {
	if (mode == m_currentPreviewChannel || mode == FLUORESCENCE_MODE::NONE) {
		// stop preview
		m_currentPreviewChannel = FLUORESCENCE_MODE::NONE;
		if (m_camera) {
			QMetaObject::invokeMethod(
				(*m_camera),
				[&m_camera = (*m_camera)]() { m_camera->stopPreview(); },
				Qt::AutoConnection
			);
		}
	} else {
		// if preview is already running, stop it first
		if (m_camera && (*m_camera)->m_isPreviewRunning) {
			(*m_camera)->stopPreview();
		}
		m_currentPreviewChannel = mode;
		ChannelSettings* channel = getChannelSettings(mode);

		// move to Fluorescence configuration
		if (m_scanControl) {
			(*m_scanControl)->setPreset(channel->preset);
		}

		// start image acquisition
		m_settings.camera.exposureTime = 1e-3 * channel->exposure;
		m_settings.camera.gain = channel->gain;
		if (m_camera) {
			(*m_camera)->setSettings(m_settings.camera);
			QMetaObject::invokeMethod(
				(*m_camera),
				[&m_camera = (*m_camera)]() { m_camera->startPreview(); },
				Qt::AutoConnection
			);
		}
	}
	emit(s_previewRunning(m_currentPreviewChannel));
}

/*
 * Private definitions
 */

void Fluorescence::abortMode(std::unique_ptr <StorageWrapper>& storage) {
	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	setAcquisitionStatus(ACQUISITION_STATUS::ABORTED);
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

	auto cameraType = std::string{};
	if (m_camera) {
		cameraType = typeid(**m_camera).name();
	}

	m_settings.camera = (*m_camera)->getSettings();

	// configure camera for measurement
	// This needs a proper implementation with user defined values. Probably by a configuration file.
	m_settings.camera.roi.left = 128;
	m_settings.camera.roi.top = 0;
	m_settings.camera.roi.width_physical = 1024;
	m_settings.camera.roi.height_physical = 1024;
	if (cameraType == "class uEyeCam") {
		m_settings.camera.roi.left = 800;
		m_settings.camera.roi.top = 400;
		m_settings.camera.roi.width_physical = 1800;
		m_settings.camera.roi.height_physical = 2000;
		m_settings.camera.readout.triggerMode = L"Software";
	} else if (cameraType == "class PointGrey") {
		m_settings.camera.roi.left = 128;
		m_settings.camera.roi.top = 0;
		m_settings.camera.roi.width_physical = 1024;
		m_settings.camera.roi.height_physical = 1024;
		m_settings.camera.readout.triggerMode = L"Software";
	}
	m_settings.camera.readout.cycleMode = L"Fixed";
	m_settings.camera.frameCount = 1;
}

std::string Fluorescence::getBinningString() {
	std::string binning{ "1x1" };
	if (m_settings.camera.roi.binning == L"8x8") {
		binning = "8x8";
	} else if (m_settings.camera.roi.binning == L"4x4") {
		binning = "4x4";
	} else if (m_settings.camera.roi.binning == L"2x2") {
		binning = "2x2";
	}
	return binning;
}

template <typename T>
void Fluorescence::__acquire(std::unique_ptr <StorageWrapper>& storage, std::vector<ChannelSettings*> channels) {
	setAcquisitionStatus(ACQUISITION_STATUS::STARTED);

	// If the provided channel settings vector is empty, acquire the enabled channels.
	if (!channels.size()) {
		channels = getEnabledChannels();
		// If no channels are enabled, return.
		if (!channels.size()) {
			return;
		}
	}

	writeScaleCalibration(storage, ACQUISITION_MODE::FLUORESCENCE);

	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->startWritingQueues(); },
		Qt::AutoConnection
	);

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	int rank_data{ 3 };
	// Loop through the different modes
	int imageNumber{ 0 };
	for (auto const& channel : channels) {
		// Abort if requested
		if (m_abort) {
			this->abortMode(storage);
			return;
		}

		// move to Fluorescence configuration
		if (m_scanControl) {
			(*m_scanControl)->setPreset(channel->preset);
		}

		/*
		 * We have to check if the exposure time or gain settings will change.
		 * If they will, then we have to trash the first image after the change,
		 * as the settings might not be applied already (valid for trigger mode).
		 * Might be worse for free running mode.
		 * See https://www.ptgrey.com/KB/10086
		 */
		auto changed{ false };
		if (((int)(1e3 * m_settings.camera.exposureTime) != channel->exposure) || (m_settings.camera.gain != channel->gain)) {
			changed = true;
		}

		m_settings.camera.exposureTime = 1e-3 * channel->exposure;
		m_settings.camera.gain = channel->gain;

		if (changed) {
			m_settings.camera.frameCount = 2;
			if (m_camera) {
				(*m_camera)->startAcquisition(m_settings.camera);
				// Settings might change after acquisition start (e.g. binning size and bytes per frame)
				m_settings.camera = (*m_camera)->getSettings();
				(*m_camera)->getImageForAcquisition(nullptr, false);
				(*m_camera)->getImageForAcquisition(nullptr, false);
				(*m_camera)->stopAcquisition();
			}
		}

		// start image acquisition
		m_settings.camera.frameCount = 1;
		if (m_camera) {
			(*m_camera)->startAcquisition(m_settings.camera);
		}

		// Settings might change after acquisition start (e.g. binning size and bytes per frame)
		if (m_camera) {
			m_settings.camera = (*m_camera)->getSettings();
		}
		hsize_t dims_data[3] = { 1, (hsize_t)m_settings.camera.roi.height_binned, (hsize_t)m_settings.camera.roi.width_binned };

		// read images from camera
		std::vector<std::byte> images(m_settings.camera.roi.bytesPerFrame);

		// acquire images
		if (m_camera) {
			(*m_camera)->getImageForAcquisition(&images[0], true);
		}

		// cast the vector to unsigned short
		auto images_ = (std::vector<T> *) & images;

		// Sometimes the uEye camera returns a black image (only zeros), we try to catch this here by
		// repeating the acquisition a maximum of 5 times
		unsigned char sum = simplemath::sum(*images_);
		int i{ 0 };
		while (sum == 0 && 5 > i++) {
			if (m_camera) {
				(*m_camera)->getImageForAcquisition(&images[0], true);
			}

			// cast the vector type T
			images_ = (std::vector<T> *) & images;

			sum = simplemath::sum(*images_);
		}

		// store images
		// asynchronously write image to disk
		// the datetime has to be set here, otherwise it would be determined by the time the queue is processed
		std::string date = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
			.toString(Qt::ISODateWithMs).toStdString();
		std::string binning = getBinningString();
		auto img = new FLUOIMAGE<T>(imageNumber, rank_data, dims_data, date, channel->name, *images_,
			m_settings.camera.exposureTime, m_settings.camera.gain, binning);

		QMetaObject::invokeMethod(
			storage.get(),
			[&storage = storage, img]() { storage.get()->s_enqueuePayload(img); },
			Qt::AutoConnection
		);

		// configure camera for preview
		if (m_camera) {
			(*m_camera)->stopAcquisition();
		}
		imageNumber++;
		double percentage = 100 * (double)imageNumber / channels.size();
		int remaining = 1e-3 * measurementTimer.elapsed() / imageNumber * ((int64_t)channels.size() - imageNumber);
		emit(s_repetitionProgress(percentage, remaining));
	}

	// Here we wait until the storage object indicate it finished to write to the file.
	QEventLoop loop;
	auto connection = QWidget::connect(
		storage.get(),
		&StorageWrapper::finished,
		&loop,
		&QEventLoop::quit
	);
	QMetaObject::invokeMethod(
		storage.get(),
		[&storage = storage]() { storage.get()->s_finishedQueueing(); },
		Qt::AutoConnection
	);
	loop.exec();

	setAcquisitionStatus(ACQUISITION_STATUS::FINISHED);
}

/*
 * Private slots
 */

void Fluorescence::acquire(std::unique_ptr <StorageWrapper>& storage) {
	acquire(storage, {});
}

void Fluorescence::acquire(std::unique_ptr<StorageWrapper>& storage, const std::vector<ChannelSettings*>& channels) {
	if (m_settings.camera.readout.dataType == "unsigned short") {
		__acquire<unsigned short>(storage, channels);
	} else if (m_settings.camera.readout.dataType == "unsigned char") {
		__acquire<unsigned char>(storage, channels);
	}
}