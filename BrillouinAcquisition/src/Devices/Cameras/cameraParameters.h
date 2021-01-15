#ifndef CAMERAPARAMETERS_H
#define CAMERAPARAMETERS_H

// possible parameters
struct CAMERA_OPTIONS {
	std::vector<std::wstring> pixelReadoutRates = { L"100 MHz", L"280 MHz" };
	std::vector<std::wstring> pixelEncodings = { L"Mono12", L"Mono12Packed", L"Mono16", L"Mono32" };
	std::vector<std::wstring> cycleModes = { L"Fixed", L"Continuous" };
	std::vector<std::wstring> triggerModes = { L"Internal", L"Software", L"External", L"External Start", L"External Exposure" };
	std::vector<std::wstring> preAmpGains = { L"12-bit (low noise)", L"12-bit (high well capacity)", L"16-bit (low noise & high well capacity)" };
	std::vector<std::wstring> imageBinnings = { L"1x1", L"2x2", L"3x3", L"4x4", L"8x8" };
	std::vector<double> exposureTimeLimits = { 0.01, 1 };
	std::vector<long long> frameCountLimits = { 1, 100 };
	std::vector<long long> ROIWidthLimits = { 1, 2 };		// minimum and maximum ROI width
	std::vector<long long> ROIHeightLimits = { 1, 2 };		// minimum and maximum ROI height
};

struct CAMERA_ROI {
	long long left{ 1 };
	long long width_physical{ 2048 };
	long long width_binned{ 2048 };
	long long top{ 1 };
	long long height_physical{ 2048 };
	long long height_binned{ 2048 };
	long long binX{ 1 };
	long long binY{ 1 };
	std::wstring binning{ L"1x1" };
	int bytesPerFrame{ -1 };
};

struct CAMERA_READOUT {
	std::wstring pixelReadoutRate{ L"100 MHz" };
	std::wstring pixelEncoding{ L"Mono16" };
	std::wstring cycleMode{ L"Continuous" };
	std::wstring triggerMode{ L"Software" };
	std::wstring preAmpGain{ L"16-bit (low noise & high well capacity)" };
};

struct CAMERA_SETTINGS {
	CAMERA_SETTINGS() {};
	CAMERA_SETTINGS(double exposureTime, double gain) : exposureTime(exposureTime), gain(gain) {};
	double exposureTime{ 0.5 };				// [s]	exposure time
	long long frameCount{ 2 };				// [1]	number of images to acquire in one sequence
	double gain{ 0 };						// [dB] gain of the camera
	bool spuriousNoiseFilter{ true };		//		turn on spurious noise filter
	CAMERA_ROI roi;							//		region of interest
	CAMERA_READOUT readout;					//		readout settings
};

typedef enum class enCameraSetting {
	EXPOSURE,
	GAIN
} CAMERA_SETTING;

#endif // CAMERAPARAMETERS_H