#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include "Device.h"
#include "../external/h5bm/TypesafeBitmask.h"

typedef enum enScanPreset {
	SCAN_NULL = 0x0,
	SCAN_BRIGHTFIELD = 0x2,
	SCAN_CALIBRATION = 0x4,
	SCAN_BRILLOUIN = 0x8,
	SCAN_EYEPIECE = 0x10,
	SCAN_ODT = 0x20,
	SCAN_EPIFLUOOFF = 0x40,
	SCAN_EPIFLUOBLUE = 0x80,
	SCAN_EPIFLUOGREEN = 0x100,
	SCAN_EPIFLUORED = 0x200,
	SCAN_LASEROFF = 0x800
} SCAN_PRESET;
ENABLE_BITMASK_OPERATORS(SCAN_PRESET)

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

struct VOLTAGE2 {
	double Ux{ 0 };
	double Uy{ 0 };
};

struct BOUNDS {
	double xMin{ -1e3 };	// [µm] minimal x-value
	double xMax{  1e3 };	// [µm] maximal x-value
	double yMin{ -1e3 };	// [µm] minimal y-value
	double yMax{  1e3 };	// [µm] maximal y-value
	double zMin{ -1e3 };	// [µm] minimal z-value
	double zMax{  1e3 };	// [µm] maximal z-value
};

class DeviceElement {
public:
	DeviceElement() {};
	DeviceElement(std::string name, int maxOptions, int index) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions)) {};
	DeviceElement(std::string name, int maxOptions, int index, std::vector<std::string> optionNames) :
		name(name), maxOptions(maxOptions), index(index), optionNames(checkNames(maxOptions, optionNames)) {};

	std::string name{ "" };
	int maxOptions{ 0 };
	int index{ 0 };
	std::vector<std::string> optionNames;

	std::vector<std::string> checkNames(int count, std::vector<std::string> names = {}) {
		//check that the number of names fits the number of options
		while (names.size() < count) {
			names.push_back(std::to_string((int)names.size() + 1));
		}
		return names;
	}
};

class DeviceElements {
private:
	std::vector<DeviceElement> m_deviceElements;

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
};

class Preset {
public:
	Preset(std::string name, SCAN_PRESET index, std::vector<std::vector<int>> positions) :
		name(name), index(index), elementPositions(positions) {};

	std::string name{ "" };
	SCAN_PRESET index{ SCAN_NULL };
	std::vector<std::vector<int>> elementPositions;
};

class ScanControl: public Device {
	Q_OBJECT

protected:
	bool m_isConnected = false;
	bool m_isCompatible = false;
	POINT3 m_homePosition = { 0, 0, 0 };

	std::vector<POINT3> m_savedPositions;

	void calculateHomePositionBounds();
	void calculateCurrentPositionBounds();

	BOUNDS m_absoluteBounds;
	BOUNDS m_homePositionBounds;
	BOUNDS m_currentPositionBounds;

	Preset getPreset(SCAN_PRESET);

public:
	ScanControl() noexcept {};
	virtual ~ScanControl() {};

	typedef enum class enScanDevice {
		ZEISSECU = 0,
		NIDAQ = 1
	} SCAN_DEVICE;
	std::vector<std::string> SCAN_DEVICE_NAMES = { "Zeiss ECU", "NI-DAQmx" };

	std::vector<DeviceElement> m_deviceElements;
	std::vector<int> m_elementPositions;

	std::vector<Preset> m_presets;
	SCAN_PRESET m_activePresets;

	bool getConnectionStatus();

	virtual void setPosition(POINT3 position) = 0;
	// moves the position relative to current position
	void movePosition(POINT3 distance);
	virtual POINT3 getPosition() = 0;

	QTimer *positionTimer = nullptr;
	QTimer *elementPositionTimer = nullptr;

public slots:
	virtual void setElement(DeviceElement, int) = 0;
	virtual void getElement(DeviceElement) = 0;
	virtual void setPreset(SCAN_PRESET) = 0;
	virtual void getElements() = 0;
	void checkPresets();
	bool isPresetActive(SCAN_PRESET);
	void announcePosition();
	void startAnnouncingPosition();
	void stopAnnouncingPosition();
	void startAnnouncingElementPosition();
	void stopAnnouncingElementPosition();
	// sets the position relative to the home position m_homePosition
	virtual void setPositionRelativeX(double position) = 0;
	virtual void setPositionRelativeY(double position) = 0;
	virtual void setPositionRelativeZ(double position) = 0;
	void setHome();
	void moveHome();
	void savePosition();
	void moveToSavedPosition(int index);
	void deleteSavedPosition(int index);
	virtual void loadVoltagePositionCalibration(std::string filepath) {};

	std::vector<POINT3> getSavedPositionsNormalized();
	void announceSavedPositionsNormalized();

signals:
	void elementPositionsChanged(std::vector<int>);
	void elementPositionChanged(DeviceElement, int);
	void currentPosition(POINT3);
	void savedPositionsChanged(std::vector<POINT3>);
	void homePositionBoundsChanged(BOUNDS);
	void currentPositionBoundsChanged(BOUNDS);
};

#endif // SCANCONTROL_H