#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include "Device.h"
#include "CalibrationHelper.h"
#include "../../external/h5bm/TypesafeBitmask.h"

enum class ScanPreset {
	SCAN_NULL			= 0x0,
	SCAN_BRIGHTFIELD	= 0x2,
	SCAN_CALIBRATION	= 0x4,
	SCAN_BRILLOUIN		= 0x8,
	SCAN_EYEPIECE		= 0x10,
	SCAN_ODT			= 0x20,
	SCAN_EPIFLUOOFF		= 0x40,
	SCAN_EPIFLUOBLUE	= 0x80,
	SCAN_EPIFLUOGREEN	= 0x100,
	SCAN_EPIFLUORED		= 0x200,
	SCAN_LASEROFF		= 0x800
};
ENABLE_BITMASK_OPERATORS(ScanPreset)

struct POINT3 {
	double x{ 0 };
	double y{ 0 };
	double z{ 0 };
	POINT3 operator+(const POINT3 &pos) {
		return POINT3{ x + pos.x, y + pos.y, z + pos.z };
	}
	POINT3 operator-(const POINT3 &pos) {
		return POINT3{ x - pos.x, y - pos.y, z - pos.z };
	}
};

struct POINT2 {
	double x{ 0 };
	double y{ 0 };
	POINT2 operator+(const POINT2 &pos) {
		return POINT2{ x + pos.x, y + pos.y };
	}
	POINT2 operator-(const POINT2 &pos) {
		return POINT2{ x - pos.x, y - pos.y };
	}
};

typedef enum class enDeviceInput {
	PUSHBUTTON,
	INTBOX,
	DOUBLEBOX,
	SLIDER
} DEVICE_INPUT_TYPE;

class DeviceElement {
public:
	DeviceElement() {};
	DeviceElement(std::string name, int maxOptions, int index) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions)) {};
	DeviceElement(std::string name, int maxOptions, int index, DEVICE_INPUT_TYPE inputType) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions)), inputType(inputType) {};
	DeviceElement(std::string name, int maxOptions, int index, std::vector<std::string> optionNames) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions, optionNames)) {};
	DeviceElement(std::string name, int maxOptions, int index, std::vector<std::string> optionNames, DEVICE_INPUT_TYPE inputType) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions, optionNames)), inputType(inputType) {};

	std::string name{ "" };
	int maxOptions{ 0 };
	int index{ 0 };
	std::vector<std::string> optionNames;
	DEVICE_INPUT_TYPE inputType{ enDeviceInput::PUSHBUTTON }; // possible types are "pushButtons", "intBox" or "doubleBox"

	std::vector<std::string> checkNames(int count, std::vector<std::string> names = {}) {
		//check that the number of names fits the number of options
		while (names.size() < count) {
			names.push_back(std::to_string((int)names.size() + 1));
		}
		return names;
	}
};

class DeviceElements {

public:
	DeviceElements() {};
	DeviceElements(std::vector<DeviceElement> deviceElements) {
		m_deviceElements = deviceElements;
	};
	void operator=(std::vector<DeviceElement> deviceElements) {
		m_deviceElements = deviceElements;
	}

	std::vector<DeviceElement> getDeviceElements() {
		return m_deviceElements;
	};

	int count() {
		return m_deviceElements.size();
	};

private:
	std::vector<DeviceElement> m_deviceElements;
};

class Preset {
public:
	Preset(std::string name, ScanPreset index, std::vector<std::vector<double>> positions) :
		name(name), index(index), elementPositions(positions) {};

	std::string name{ "" };
	ScanPreset index{ ScanPreset::SCAN_NULL };
	std::vector<std::vector<double>> elementPositions;
};

class ScanControl: public Device {
	Q_OBJECT

public:
	ScanControl() noexcept {};
	virtual ~ScanControl() {};

	bool getConnectionStatus();

	virtual void setPosition(POINT2 position) = 0;
	virtual void setPosition(POINT3 position) = 0;
	// moves the position relative to current position
	void movePosition(POINT3 distance);
	virtual POINT3 getPosition() = 0;

	typedef enum class enScanDevice {
		ZEISSECU = 0,
		NIDAQ = 1,
		ZEISSMTB = 2,
		ZEISSMTBERLANGEN = 3
	} SCAN_DEVICE;
	inline static std::vector<std::string> SCAN_DEVICE_NAMES = { "Zeiss ECU", "NI-DAQmx", "Zeiss MTB", "Zeiss MTB Erlangen" };

	std::vector<DeviceElement> m_deviceElements;
	std::vector<double> m_elementPositions;
	std::vector<double> m_elementPositionsTmp = m_elementPositions;

	std::vector<Preset> m_presets;
	ScanPreset m_activePresets = ScanPreset::SCAN_NULL;

public slots:
	virtual void init() = 0;
	virtual void connectDevice() = 0;
	virtual void disconnectDevice() = 0;
	virtual void setElement(DeviceElement, double) = 0;
	virtual int getElement(DeviceElement) = 0;
	virtual void getElements() = 0;
	virtual void setPreset(ScanPreset) = 0;
	// sets the position relative to the home position m_homePosition
	virtual void setPositionRelativeX(double position) = 0;
	virtual void setPositionRelativeY(double position) = 0;
	virtual void setPositionRelativeZ(double position) = 0;
	virtual void setPositionInPix(POINT2) = 0;

	Preset getPreset(ScanPreset);
	void checkPresets();
	bool isPresetActive(ScanPreset);
	void announcePosition();
	void startAnnouncing();
	void stopAnnouncing();
	void startAnnouncingPosition();
	void stopAnnouncingPosition();
	void startAnnouncingElementPosition();
	void stopAnnouncingElementPosition();
	void setHome();
	void moveHome();
	void savePosition();
	void moveToSavedPosition(int index);
	void deleteSavedPosition(int index);
	virtual void setSpatialCalibration(SpatialCalibration spatialCalibration) {};

	std::vector<POINT3> getSavedPositionsNormalized();
	void announceSavedPositionsNormalized();

	bool hasSpatialCalibration();

protected:
	virtual POINT2 pixToMicroMeter(POINT2) = 0;

	void calculateHomePositionBounds();
	void calculateCurrentPositionBounds();
	void calculateCurrentPositionBounds(POINT3 currentPosition);

	bool m_isConnected{ false };
	bool m_isCompatible{ false };
	POINT3 m_homePosition{ 0, 0, 0 };

	bool m_hasSpatialCalibration{ false };

	std::vector<POINT3> m_savedPositions;

	QTimer* positionTimer{ nullptr };
	QTimer* elementPositionTimer{ nullptr };

	BOUNDS m_absoluteBounds;
	BOUNDS m_homePositionBounds;
	BOUNDS m_currentPositionBounds;

	SpatialCalibration m_calibration;

signals:
	void elementPositionsChanged(std::vector<double>);
	void elementPositionChanged(DeviceElement, double);
	void currentPosition(POINT3);
	void savedPositionsChanged(std::vector<POINT3>);
	void homePositionBoundsChanged(BOUNDS);
	void currentPositionBoundsChanged(BOUNDS);
};

#endif // SCANCONTROL_H