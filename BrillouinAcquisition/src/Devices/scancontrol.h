#ifndef SCANCONTROL_H
#define SCANCONTROL_H

#include "Device.h"

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

struct DeviceElement {
	std::string name{ "" };
	int maxOptions{ 0 };
	int index{ 0 };
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

	int deviceCount() {
		return m_deviceElements.size();
	};
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

public:
	ScanControl() noexcept {};
	virtual ~ScanControl() {};

	typedef enum class enScanDevice {
		ZEISSECU = 0,
		NIDAQ = 1
	} SCAN_DEVICE;
	std::vector<std::string> SCAN_DEVICE_NAMES = { "Zeiss ECU", "NI-DAQmx" };

	// pre-defined presets for element positions
	std::vector<std::string> m_presetLabels = { "Brightfield", "Calibration", "Brillouin", "Eyepiece" };
	typedef enum enScanPreset {
		SCAN_BRIGHTFIELD,
		SCAN_CALIBRATION,
		SCAN_BRILLOUIN,
		SCAN_EYEPIECE,
		SCAN_PRESET_COUNT
	} SCAN_PRESET;

	std::vector<std::vector<int>> m_presets;
	std::vector<int> m_availablePresets;

	DeviceElements m_deviceElements;

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
	virtual void setElements(ScanControl::SCAN_PRESET) = 0;
	virtual void getElements() = 0;
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