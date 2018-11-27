#ifndef CAMERAPARAMETERS_H
#define CAMERAPARAMETERS_H

#include "atcore.h"

// possible parameters
struct CAMERA_OPTIONS {
	std::vector<std::wstring> pixelReadoutRates = { L"100 MHz", L"280 MHz" };
	std::vector<std::wstring> pixelEncodings = { L"Mono12", L"Mono12Packed", L"Mono16", L"Mono32" };
	std::vector<std::wstring> cycleModes = { L"Fixed", L"Continuous" };
	std::vector<std::wstring> triggerModes = { L"Internal", L"Software", L"External", L"External Start", L"External Exposure" };
	std::vector<std::wstring> preAmpGains = { L"12-bit (low noise)", L"12-bit (high well capacity)", L"16-bit (low noise & high well capacity)" };
	std::vector<std::wstring> imageBinnings = { L"1x1", L"2x2", L"3x3", L"4x4", L"8x8" };
	std::vector<double> exposureTimeLimits = { 0.01, 1 };
	std::vector<AT_64> frameCountLimits = { 1, 100 };
	std::vector<AT_64> ROIWidthLimits = { 1, 2 };		// minimum and maximum ROI width
	std::vector<AT_64> ROIHeightLimits = { 1, 2 };		// minimum and maximum ROI height
};

struct CAMERA_ROI {
	AT_64 left = 1;
	AT_64 width = 2048;
	AT_64 top = 1;
	AT_64 height = 2048;
	std::wstring binning{ L"1x1" };
};

struct CAMERA_READOUT {
	std::wstring pixelReadoutRate{ L"100 MHz" };
	std::wstring pixelEncoding{ L"Mono16" };
	std::wstring cycleMode{ L"Continuous" };
	std::wstring triggerMode{ L"Software" };
	std::wstring preAmpGain{ L"16-bit (low noise & high well capacity)" };
};

struct CAMERA_SETTINGS {
	double exposureTime = 0.5;				// [s]	exposure time
	AT_64 frameCount = 2;					// [1]	number of images to acquire in one sequence
	double gain{ 0 };						// [dB] gain of the camera
	AT_BOOL spuriousNoiseFilter = true;		//		turn on spurious noise filter
	CAMERA_ROI roi;							//		region of interest
	CAMERA_READOUT readout;					//		readout settings
};

typedef enum class enCameraSetting {
	EXPOSURE,
	GAIN
} CAMERA_SETTING;

#endif // CAMERAPARAMETERS_H