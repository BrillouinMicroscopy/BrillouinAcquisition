#include "stdafx.h"
#include "pvcamera.h"

/*
 * Public definitions
 */

PVCamera::~PVCamera() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	disconnectDevice();
	if (m_isInitialised) {
		PVCam::pl_pvcam_uninit();
	}
}

/*
 * Public slots
 */

void PVCamera::init() {
	// create timers and connect their signals
	// after moving camera to another thread
	m_tempTimer = new QTimer();
	auto connection = QWidget::connect(
		m_tempTimer,
		&QTimer::timeout,
		this,
		&PVCamera::checkSensorTemperature
	);
}

void PVCamera::connectDevice() {
	// initialize library
	initialize();
	if (!m_isConnected && m_isInitialised) {
		char g_Camera0_Name[CAM_NAME_LEN] = "";
		PVCam::pl_cam_get_name(0, g_Camera0_Name);
		auto i_retCode = PVCam::pl_cam_open(g_Camera0_Name, &m_camera, PVCam::OPEN_EXCLUSIVE);
		if (i_retCode == PVCam::PV_OK) {
			m_isConnected = true;
			readOptions();
			setSettings(m_settings);
			startTempTimer();
		}
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::disconnectDevice() {
	if (m_isConnected) {
		stopTempTimer();
		auto i_retCode = PVCam::pl_cam_close(m_camera);
		if (i_retCode == PVCam::PV_OK) {
			m_isConnected = false;
		}
	}
	emit(connectedDevice(m_isConnected));
}

void PVCamera::setSettings(CAMERA_SETTINGS settings) {
	// We have to update the options when we change port and speed
	auto updateOptions{ false };
	if (m_settings.readout.pixelReadoutRate != settings.readout.pixelReadoutRate) {
		updateOptions = true;
	}

	m_settings = settings;

	auto binning{ 1 };
	if (m_settings.roi.binning == L"8x8") {
		binning = 8;
	} else if (m_settings.roi.binning == L"4x4") {
		binning = 4;
	} else if (m_settings.roi.binning == L"2x2") {
		binning = 2;
	} else if (m_settings.roi.binning == L"1x1") {
		binning = 1;
	} else {
		// Fallback to 1x1 binning
		m_settings.roi.binning = L"1x1";
	}
	m_settings.roi.binX = binning;
	m_settings.roi.binY = binning;

	// Verify that the image size is a multiple of the binning number
	auto modx = m_settings.roi.width_physical % m_settings.roi.binX;
	if (modx) {
		m_settings.roi.width_physical -= modx;
	}
	auto mody = m_settings.roi.height_physical % m_settings.roi.binY;
	if (mody) {
		m_settings.roi.height_physical -= mody;
	}

	m_settings.roi.width_binned = m_settings.roi.width_physical / m_settings.roi.binX;
	m_settings.roi.height_binned = m_settings.roi.height_physical / m_settings.roi.binY;

	auto speedTableIndex{ 0 };
	for (gsl::index i{ 0 }; i < m_SpeedTable.size(); i++) {
		if (m_SpeedTable[i].label == m_settings.readout.pixelReadoutRate) {
			speedTableIndex = i;
			break;
		}
	}

	auto gainIndex{ 0 };
	for (gsl::index i{ 0 }; i < m_SpeedTable[speedTableIndex].gains.size(); i++) {
		if (std::to_wstring(m_SpeedTable[speedTableIndex].gains[i]) == m_settings.readout.preAmpGain) {
			gainIndex = i;
			break;
		}
	}

	// Set camera to first port
	if (PVCam::PV_OK != PVCam::pl_set_param(m_camera, PARAM_READOUT_PORT,
		(void*)&m_SpeedTable[speedTableIndex].port.value)) {
		//PrintErrorMessage(pl_error_code(), "Readout port could not be set");
		//return false;
	}
	//printf("Setting readout port to %s\n", m_SpeedTable[0].port.name.c_str());

	// Set camera to speed 0
	if (PVCam::PV_OK != PVCam::pl_set_param(m_camera, PARAM_SPDTAB_INDEX,
		(void*)&m_SpeedTable[speedTableIndex].speedIndex)) {
		//PrintErrorMessage(pl_error_code(), "Readout port could not be set");
		//return false;
	}
	//printf("Setting readout speed index to %d\n", m_SpeedTable[0].speedIndex);

	// Set gain index to one (the first one)
	if (PVCam::PV_OK != PVCam::pl_set_param(m_camera, PARAM_GAIN_INDEX,
		(void*)&m_SpeedTable[speedTableIndex].gains[gainIndex])) {
		//PrintErrorMessage(pl_error_code(), "Gain index could not be set");
		//return false;
	}

	auto camSettings = getCamSettings();
	auto bufferSize = PVCam::uns32{};
	auto i_retCode = PVCam::pl_exp_setup_cont(
		m_camera,
		1,
		&camSettings,
		PVCam::TIMED_MODE,
		1e3 * m_settings.exposureTime,
		&bufferSize,
		PVCam::CIRC_OVERWRITE
	);
	m_settings.roi.bytesPerFrame = bufferSize;

	// read options as they might change with port and speed
	if (updateOptions) {
		readOptions();
	}

	// read back the settings
	readSettings();
}

void PVCamera::startPreview() {
	// don't do anything if an acquisition is running
	if (m_isAcquisitionRunning) {
		return;
	}
	m_isPreviewRunning = true;
	m_stopPreview = false;
	preparePreview();

	emit(s_previewRunning(m_isPreviewRunning));
}

void PVCamera::stopPreview() {
	cleanupPreview();
	m_isPreviewRunning = false;
	m_stopPreview = false;
	emit(s_previewRunning(m_isPreviewRunning));
}

void PVCamera::startAcquisition(CAMERA_SETTINGS settings) {
	prepareAcquisition(settings);

	m_isAcquisitionRunning = true;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void PVCamera::stopAcquisition() {
	cleanupAcquisition();
	m_isAcquisitionRunning = false;
	emit(s_acquisitionRunning(m_isAcquisitionRunning));
}

void PVCamera::getImageForAcquisition(unsigned char* buffer, bool preview) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	auto i_retCode = PVCam::pl_exp_start_seq(m_camera, m_acquisitionBuffer);

	{
		std::unique_lock<std::mutex> lock(g_EofMutex);
		if (!g_EofFlag) {
			auto waitTime = (int)(2 * m_settings.exposureTime);
			waitTime = (waitTime < 5) ? 5 : waitTime;
			g_EofCond.wait_for(lock, std::chrono::seconds(waitTime), [this]() {
				return (g_EofFlag);
				});
		}
		g_EofFlag = false; // Reset flag
	}

	memcpy(buffer, m_acquisitionBuffer, m_settings.roi.bytesPerFrame);

	PVCam::pl_exp_finish_seq(m_camera, m_acquisitionBuffer, 0);

	if (preview) {
		// write image to preview buffer
		memcpy(m_previewBuffer->m_buffer->getWriteBuffer(), buffer, m_settings.roi.bytesPerFrame);
		m_previewBuffer->m_buffer->m_usedBuffers->release();
	}
}

void PVCamera::setCalibrationExposureTime(double exposureTime) {
	PVCam::pl_exp_abort(m_camera, PVCam::CCS_NO_CHANGE);

	PVCam::pl_cam_register_callback_ex3(m_camera, PVCam::PL_CALLBACK_EOF, (void*)acquisitionCallback, (void*)this);

	// Set the exposure time
	m_settings.exposureTime = exposureTime;
	setSettings(m_settings);
}

void PVCamera::setSensorCooling(bool cooling) {
	if (cooling) {
		//m_sensorTemperature.setpoint = m_sensorTemperature.minSetpoint;
		// TODO: We set the temperature to -15 �C for now, so we don't stress
		// the camera to much while developing.
		m_sensorTemperature.setpoint = -20.0;
	} else {
		auto setpoint = m_sensorTemperature.maxSetpoint;
		// We want to set the value no higher than room temperature.
		if (setpoint > 20) {
			setpoint = 20;
		}
		m_sensorTemperature.setpoint = setpoint;
	}
	auto setpoint{ (PVCam::int16)(100 * m_sensorTemperature.setpoint) };
	auto i_retCode = PVCam::pl_set_param(m_camera, PARAM_TEMP_SETPOINT, (void*)&setpoint);
	m_isCooling = cooling;
	emit(cameraCoolingChanged(m_isCooling));
}

bool PVCamera::getSensorCooling() {
	auto setpoint = PVCam::int16{ 0 };
	auto i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_CURRENT, (void*)&setpoint);
	// If the setpoint is lower than 0 �C we consider it cooling.
	if (setpoint / 100.0 < 0.0) {
		return true;
	} else {
		return false;
	}
}

/*
 * Private definitions
 */

int PVCamera::acquireImage(unsigned char* buffer) {
	auto status = PVCam::int16{};
	auto byte_cnt = PVCam::uns32{};
	auto buffer_cnt = PVCam::uns32{};
	while (PVCam::pl_exp_check_cont_status(m_camera, &status, &byte_cnt, &buffer_cnt)
		&& status != PVCam::FRAME_AVAILABLE && status != PVCam::READOUT_NOT_ACTIVE) {
		// Waiting for frame exposure and readout
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (status == PVCam::READOUT_NOT_ACTIVE) {
		return 0;
	}

	PVCam::uns16* frameAddress { nullptr };
	PVCam::pl_exp_get_latest_frame(m_camera, (void**)&frameAddress);
	memcpy(buffer, frameAddress, m_settings.roi.bytesPerFrame);

	return 1;
}

void PVCamera::readOptions() {
	auto i_retCode{ 0 };
	// Read min and max temperature setpoint
	auto setpointMin = PVCam::int16{ 0 };
	i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_MIN, (void*)&setpointMin);
	if (i_retCode == PVCam::PV_OK) {
		m_sensorTemperature.minSetpoint = setpointMin / 100.0;
	}

	auto setpointMax = PVCam::int16{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_TEMP_SETPOINT, PVCam::ATTR_MAX, (void*)&setpointMax);
	if (i_retCode == PVCam::PV_OK) {
		m_sensorTemperature.maxSetpoint = setpointMax / 100.0;
	}


	auto ROIHeight = PVCam::int16{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_PAR_SIZE, PVCam::ATTR_CURRENT, (void*)&ROIHeight);
	m_options.ROIHeightLimits[0] = 0;
	m_options.ROIHeightLimits[1] = ROIHeight;

	auto ROIWidth = PVCam::int16{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_SER_SIZE, PVCam::ATTR_CURRENT, (void*)&ROIWidth);
	m_options.ROIWidthLimits[0] = 0;
	m_options.ROIWidthLimits[1] = ROIWidth;

	auto exposureMin = PVCam::int16{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_EXPOSURE_TIME, PVCam::ATTR_MIN, (void*)&exposureMin);
	m_options.exposureTimeLimits[0] = 1e-3 * (double)exposureMin;

	auto exposureMax = PVCam::int16{ 0 };
	PVCam::pl_get_param(m_camera, PARAM_EXPOSURE_TIME, PVCam::ATTR_MAX, (void*)&exposureMax);
	m_options.exposureTimeLimits[1] = 1e-3 * (double)exposureMax;

	/*
	 * Get the possible binning factors
	 */
	auto isAvailable = PVCam::rs_bool{ false };
	PVCam::pl_get_param(m_camera, PARAM_BINNING_PAR, PVCam::ATTR_AVAIL, (void*)&isAvailable);
	if (isAvailable) {
		auto binsSer = NVPC{};
		ReadEnumeration(&binsSer, PARAM_BINNING_SER, "PARAM_BINNING_SER");
		auto binsPar = NVPC{};
		ReadEnumeration(&binsPar, PARAM_BINNING_PAR, "PARAM_BINNING_PAR");
		const auto binCount = (PVCam::uns32)std::min<size_t>(binsSer.size(), binsPar.size());
		auto i{ 0 };
		m_options.imageBinnings.clear();
		for (PVCam::uns32 n{ 0 }; n < binCount; n++) {
			std::wstring string = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(binsSer[n].name);
			m_options.imageBinnings.push_back(string);
		}
	}

	auto ports = NVPC{};
	ReadEnumeration(&ports, PARAM_READOUT_PORT, "PARAM_READOUT_PORT");

	m_SpeedTable.clear();
	// Iterate through available ports and their speeds
	for (size_t pi{ 0 }; pi < ports.size(); pi++) {
		// Set readout port
		if (PVCam::PV_OK != PVCam::pl_set_param(m_camera, PARAM_READOUT_PORT,
			(void*)&ports[pi].value)) {
			//PrintErrorMessage(pl_error_code(),
			//	"pl_set_param(PARAM_READOUT_PORT) error");
			//return false;
		}

		// Get number of available speeds for this port
		auto speedCount = PVCam::uns32{};
		if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_SPDTAB_INDEX, PVCam::ATTR_COUNT,
			(void*)&speedCount)) {
			//PrintErrorMessage(pl_error_code(),
			//	"pl_get_param(PARAM_SPDTAB_INDEX) error");
			//return false;
		}

		// Iterate through all the speeds
		for (PVCam::int16 si = 0; si < (PVCam::int16)speedCount; si++) {
			// Set camera to new speed index
			if (PVCam::PV_OK != PVCam::pl_set_param(m_camera, PARAM_SPDTAB_INDEX, (void*)&si)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_set_param(g_hCam, PARAM_SPDTAB_INDEX) error");
				//return false;
			}

			// Get pixel time (readout time of one pixel in nanoseconds) for the
			// current port/speed pair. This can be used to calculate readout
			// frequency of the port/speed pair.
			auto pixTime = PVCam::uns16{};
			if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_PIX_TIME, PVCam::ATTR_CURRENT,
				(void*)&pixTime)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_get_param(g_hCam, PARAM_PIX_TIME) error");
				//return false;
			}

			// Get bit depth of the current readout port/speed pair
			auto bitDepth = PVCam::int16{};
			if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_BIT_DEPTH, PVCam::ATTR_CURRENT,
				(void*)&bitDepth)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_get_param(PARAM_BIT_DEPTH) error");
				//return false;
			}

			auto gainMin = PVCam::int16{};
			if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_GAIN_INDEX, PVCam::ATTR_MIN,
				(void*)&gainMin)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_get_param(PARAM_GAIN_INDEX) error");
				//return false;
			}

			auto gainMax = PVCam::int16{};
			if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_GAIN_INDEX, PVCam::ATTR_MAX,
				(void*)&gainMax)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_get_param(PARAM_GAIN_INDEX) error");
				//return false;
			}

			auto gainIncrement = PVCam::int16{};
			if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, PARAM_GAIN_INDEX, PVCam::ATTR_INCREMENT,
				(void*)&gainIncrement)) {
				//PrintErrorMessage(pl_error_code(),
				//	"pl_get_param(PARAM_GAIN_INDEX) error");
				//return false;
			}

			// Save the port/speed information to our Speed Table
			auto ro = READOUT_OPTION{};
			ro.port = ports[pi];
			ro.speedIndex = si;
			ro.readoutFrequency = 1000 / (float)pixTime;
			ro.bitDepth = bitDepth;
			auto ss = std::ostringstream{};
			ss << "P" << ro.port.value << "S" << ro.speedIndex
				<< ": " << ro.readoutFrequency << " MHz";
			ro.label = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ss.str());

			ro.gains.clear();
			auto gainValue = gainMin;
			while (gainValue <= gainMax) {
				ro.gains.push_back(gainValue);
				gainValue += gainIncrement;
			}

			m_SpeedTable.push_back(ro);
		}
	}

	m_options.pixelReadoutRates.clear();
	for (gsl::index i{ 0 }; i < m_SpeedTable.size(); i++) {
		m_options.pixelReadoutRates.push_back(m_SpeedTable[i].label);
	}

	int speedTableIndex{ 0 };
	for (gsl::index i{ 0 }; i < m_SpeedTable.size(); i++) {
		if (m_SpeedTable[i].label == m_settings.readout.pixelReadoutRate) {
			speedTableIndex = i;
			break;
		}
	}
	m_options.pixelEncodings.clear();
	auto ss = std::ostringstream{};
	ss << m_SpeedTable[speedTableIndex].bitDepth << " bit";
	m_options.pixelEncodings.push_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ss.str()));

	m_options.preAmpGains.clear();
	for (gsl::index i{ 0 }; i < m_SpeedTable[speedTableIndex].gains.size(); i++) {
		m_options.preAmpGains.push_back(std::to_wstring(m_SpeedTable[speedTableIndex].gains[i]));
	}

	emit(optionsChanged(m_options));
}

void PVCamera::readSettings() {
	// emit signal that settings changed
	emit(settingsChanged(m_settings));
}

bool PVCamera::initialize() {
	if (!m_isInitialised) {
		auto i_retCode = PVCam::pl_pvcam_init();
		if (i_retCode != PVCam::PV_OK) {
			//error condition
			//PrintErrorMessage(PVCam::pl_error_code(), "pl_pvcam_init() error");
			m_isInitialised = false;
		} else {
			auto i_numberOfCameras = PVCam::int16{ 0 };
			PVCam::pl_cam_get_total(&i_numberOfCameras);
			if (i_numberOfCameras > 0) {
				m_isInitialised = true;
			} else {
				emit(noCameraFound());
				PVCam::pl_pvcam_uninit();
				m_isInitialised = false;
			}
		}
	}
	return m_isInitialised;
}

const std::string PVCamera::getTemperatureStatus() {
	// If the setpoint is above 0, we consider it not cooling
	if (m_sensorTemperature.setpoint >= 0) {
		return "Cooler Off";
	}
	// If sensor temperature and setpoint differ no more than 1 K
	// we consider the temperature stabilised.
	auto temp = getSensorTemperature();
	if (abs(temp - m_sensorTemperature.setpoint) < 1.0) {
		return "Stabilised";
	}
	return "Cooling";
}

double PVCamera::getSensorTemperature() {
	auto temperature = PVCam::int16{};
	auto i_retCode = PVCam::pl_get_param(m_camera, PARAM_TEMP, PVCam::ATTR_CURRENT, (void*)&temperature);
	return temperature / 100.0;
}

void PVCamera::previewCallback(PVCam::FRAME_INFO* pFrameInfo, void* context) {
	auto self = static_cast<PVCamera*>(context);
	self->getImageForPreview();
}

void PVCamera::acquisitionCallback(PVCam::FRAME_INFO* pFrameInfo, void* context) {
	auto self = static_cast<PVCamera*>(context);
	{
		std::lock_guard<std::mutex> lock(self->g_EofMutex);
		self->g_EofFlag = true; // Set flag
	}
	self->g_EofCond.notify_one();
}

void PVCamera::startTempTimer() {
	if (!m_tempTimer->isActive()) {
		QMetaObject::invokeMethod(this, [this]() { m_tempTimer->start(1000); }, Qt::AutoConnection);
	}
	while (!m_tempTimer->isActive()) {
		Sleep(10);
	}
}

void PVCamera::stopTempTimer() {
	if (m_tempTimer->isActive()) {
		QMetaObject::invokeMethod(this, [this]() { m_tempTimer->stop(); }, Qt::AutoConnection);
	}
	while (m_tempTimer->isActive()) {
		Sleep(10);
	}
}

bool PVCamera::IsParamAvailable(PVCam::uns32 paramID, const char* paramName) {
	if (paramName == NULL) {
		return false;
	}

	auto isAvailable = PVCam::rs_bool{};
	if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, paramID, PVCam::ATTR_AVAIL, (void*)&isAvailable)) {
		//printf("Error reading ATTR_AVAIL of %s\n", paramName);
		return false;
	}
	if (isAvailable == false) {
		//printf("Parameter %s is not available\n", paramName);
		return false;
	}

	return true;
}

bool PVCamera::ReadEnumeration(NVPC* nvpc, PVCam::uns32 paramID, const char* paramName) {
	if (nvpc == NULL || paramName == NULL)
		return false;

	if (!IsParamAvailable(paramID, paramName))
		return false;

	auto count = PVCam::uns32{};
	if (PVCam::PV_OK != PVCam::pl_get_param(m_camera, paramID, PVCam::ATTR_COUNT, (void*)&count)) {
		//const std::string msg =
		//	"pl_get_param(" + std::string(paramName) + ") error";
		//PrintErrorMessage(pl_error_code(), msg.c_str());
		return false;
	}

	// Actually get the triggering/exposure names
	for (PVCam::uns32 i{ 0 }; i < count; ++i) {
		// Ask how long the string is
		auto strLength = PVCam::uns32{};
		if (PVCam::PV_OK != PVCam::pl_enum_str_length(m_camera, paramID, i, &strLength)) {
			//const std::string msg =
			//	"pl_enum_str_length(" + std::string(paramName) + ") error";
			//PrintErrorMessage(pl_error_code(), msg.c_str());
			return false;
		}

		// Allocate the destination string
		auto name = new (std::nothrow) char[strLength];
		if (name) {

			// Actually get the string and value
			PVCam::int32 value;
			if (PVCam::PV_OK != PVCam::pl_get_enum_param(m_camera, paramID, i, &value, name, strLength)) {
				//const std::string msg =
				//	"pl_get_enum_param(" + std::string(paramName) + ") error";
				//PrintErrorMessage(pl_error_code(), msg.c_str());
				delete[] name;
				return false;
			}

			auto nvp = NVP{};
			nvp.value = value;
			nvp.name = name;
			nvpc->push_back(nvp);
		}

		delete[] name;
	}

	return !nvpc->empty();
}

PVCam::rgn_type PVCamera::getCamSettings() {
	auto camSettings = PVCam::rgn_type{};
	camSettings.s1 = m_settings.roi.left - 1;
	camSettings.s2 = m_settings.roi.width_physical + m_settings.roi.left - 2;
	camSettings.p1 = m_settings.roi.top - 1;
	camSettings.p2 = m_settings.roi.height_physical + m_settings.roi.top - 2;
	camSettings.sbin = m_settings.roi.binY;
	camSettings.pbin = m_settings.roi.binX;
	return camSettings;
}

/*
 * Private slots
 */

void PVCamera::getImageForPreview() {
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (m_isPreviewRunning) {
		if (m_stopPreview) {
			stopPreview();
			return;
		}

		// if no image is ready return immediately
		if (!m_previewBuffer->m_buffer->m_freeBuffers->tryAcquire()) {
			Sleep(50);
			return;
		}

		acquireImage(m_previewBuffer->m_buffer->getWriteBuffer());
		m_previewBuffer->m_buffer->m_usedBuffers->release();
	}
}

void PVCamera::preparePreview() {
	// Disable temperature timer if it is running
	stopTempTimer();

	// always use full camera image for live preview
	m_settings.roi.width_physical = m_options.ROIWidthLimits[1];
	m_settings.roi.left = 1;
	m_settings.roi.height_physical = m_options.ROIHeightLimits[1];
	m_settings.roi.top = 1;

	setSettings(m_settings);

	// preview buffer
	auto circBufferFrames{ 8 };
	// Only even numbers are accepted for the frame count.
	if (circBufferFrames % 2) {
		circBufferFrames += 1;
	}
	auto bufferSettings = BUFFER_SETTINGS{ circBufferFrames, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned short", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());

	// internal buffer
	if (m_buffer) {
		delete[] m_buffer;
		m_buffer = nullptr;
	}
	auto bufSize = (size_t)circBufferFrames * m_settings.roi.bytesPerFrame / sizeof(PVCam::uns16);
	m_buffer = new (std::nothrow) PVCam::uns16[bufSize];

	// Register callback
	PVCam::pl_cam_register_callback_ex3(m_camera, PVCam::PL_CALLBACK_EOF,
		(void*)&previewCallback, (void*)this);

	// Start acquisition
	PVCam::pl_exp_start_cont(m_camera, m_buffer, bufSize);
}

void PVCamera::cleanupPreview() {
	PVCam::pl_exp_stop_cont(m_camera, PVCam::CCS_CLEAR);
	startTempTimer();
}

void PVCamera::prepareAcquisition(CAMERA_SETTINGS settings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	// check if currently a preview is running and stop it in case
	if (m_isPreviewRunning) {
		stopPreview();
	}

	// Disable temperature timer if it is running
	stopTempTimer();

	PVCam::pl_cam_register_callback_ex3(m_camera, PVCam::PL_CALLBACK_EOF, (void*)acquisitionCallback, (void*)this);
	
	setSettings(settings);

	m_acquisitionBuffer = new (std::nothrow) PVCam::uns16[m_settings.roi.bytesPerFrame / sizeof(PVCam::uns16)];

	auto bufferSettings = BUFFER_SETTINGS{ 8, (unsigned int)m_settings.roi.bytesPerFrame, "unsigned short", m_settings.roi };
	m_previewBuffer->initializeBuffer(bufferSettings);
	emit(s_previewBufferSettingsChanged());
}

void PVCamera::cleanupAcquisition() {
	PVCam::pl_exp_abort(m_camera, PVCam::CCS_NO_CHANGE);
	startTempTimer();
}

void PVCamera::checkSensorTemperature() {
	m_sensorTemperature.temperature = getSensorTemperature();
	auto status = getTemperatureStatus();
	if (status == "Cooler Off") {
		m_sensorTemperature.status = enCameraTemperatureStatus::COOLER_OFF;
	} else if (status == "Cooling") {
		m_sensorTemperature.status = enCameraTemperatureStatus::COOLING;
	} else if (status == "Stabilised") {
		m_sensorTemperature.status = enCameraTemperatureStatus::STABILISED;
	} else {
		m_sensorTemperature.status = enCameraTemperatureStatus::FAULT;
	}
	emit(s_sensorTemperatureChanged(m_sensorTemperature));
}