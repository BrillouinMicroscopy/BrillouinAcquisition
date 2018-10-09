#include "stdafx.h"
#include "Fluorescence.h"

Fluorescence::Fluorescence(QObject *parent, Acquisition *acquisition, PointGrey **pointGrey, NIDAQ **nidaq)
	: AcquisitionMode(parent, acquisition), m_pointGrey(pointGrey), m_NIDAQ(nidaq) {}

Fluorescence::~Fluorescence() {
}

void Fluorescence::initialize() {
	m_settings.camera.gain = 10;
	emit(s_acqSettingsChanged(m_settings));
}

void Fluorescence::setGain(int gain) {
	m_settings.camera.gain = gain;
	emit(s_acqSettingsChanged(m_settings));
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

std::vector<ChannelSettings *> Fluorescence::getEnabledChannels() {
	std::vector<ChannelSettings *> enabledChannels;
	for (gsl::index i{ 0 }; i < (int)FLUORESCENCE_MODE::MODE_COUNT; i++) {

		ChannelSettings *channel = getChannelSettings((FLUORESCENCE_MODE)i);
		// Don't acquire this mode if it is not enabled
		if (!channel->enabled) {
			continue;
		} else {
			enabledChannels.push_back(channel);
		}
	}
	return enabledChannels;
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

void Fluorescence::startRepetitions() {
	// don't do anything if no channels are enabled
	auto enabledChannels = getEnabledChannels();
	if (!enabledChannels.size()) {
		return;
	}

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

	m_acquisition->newRepetition(ACQUISITION_MODE::FLUORESCENCE);

	// start repetition
	acquire(m_acquisition->m_storage);

	m_acquisition->disableMode(ACQUISITION_MODE::FLUORESCENCE);
}

void Fluorescence::acquire(std::unique_ptr <StorageWrapper> & storage) {
	m_status = ACQUISITION_STATUS::STARTED;
	emit(s_acquisitionStatus(m_status));

	auto enabledChannels = getEnabledChannels();

	QElapsedTimer measurementTimer;
	measurementTimer.start();

	int rank_data{ 3 };
	hsize_t dims_data[3] = { 1, m_settings.camera.roi.height, m_settings.camera.roi.width };
	int bytesPerFrame = m_settings.camera.roi.width * m_settings.camera.roi.height;
	// Loop through the different modes
	int imageNumber{ 0 };
	for (auto const& channel : enabledChannels) {
		// Abort if requested
		if (m_abort) {
			this->abortMode();
			return;
		}

		// Don't acquire this mode if it is not enabled
		if (!channel->enabled) {
			continue;
		}

		m_settings.camera.exposureTime = 1e-3*channel->exposure;
		(*m_pointGrey)->startAcquisition(m_settings.camera);

		// move to Fluorescence configuration
		(*m_NIDAQ)->setPreset(channel->preset);

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
		FLUOIMAGE *img = new FLUOIMAGE(imageNumber, rank_data, dims_data, date, channel->name, *images_);

		QMetaObject::invokeMethod(storage.get(), "s_enqueuePayload", Qt::AutoConnection, Q_ARG(FLUOIMAGE*, img));

		// configure camera for preview
		(*m_pointGrey)->stopAcquisition();
		imageNumber++;
		double percentage = 100 * (double)imageNumber / enabledChannels.size();
		int remaining = 1e-3 * measurementTimer.elapsed() / imageNumber * ((int64_t)enabledChannels.size() - imageNumber);
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
