#ifndef SCANCONTROL_H
#define SCANCONTROL_H

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

struct BOUNDS {
	double xMin{ -1e3 };	// [µm] minimal x-value
	double xMax{  1e3 };	// [µm] maximal x-value
	double yMin{ -1e3 };	// [µm] minimal y-value
	double yMax{  1e3 };	// [µm] maximal y-value
	double zMin{ -1e3 };	// [µm] minimal z-value
	double zMax{  1e3 };	// [µm] maximal z-value
};

class ScanControl: public QObject {
	Q_OBJECT

protected:
	bool m_isConnected = false;
	bool m_isCompatible = false;
	POINT3 m_homePosition = { 0,0,0 };

	std::vector<POINT3> m_savedPositions;

	BOUNDS m_normalizedBounds;

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

	std::vector<std::vector<int>> m_presets = {
		{ 1, 1, 3, 1, 2, 2 },	// Brightfield
		{ 1, 1, 3, 1, 3, 2 },	// Calibration
		{ 1, 1, 3, 1, 2, 1 },	// Brillouin
		{ 1, 1, 3, 2, 3, 2 }	// Eyepiece
	};
	std::vector<int> m_availablePresets;

	std::vector<std::string> m_groupLabels = { "Reflector", "Objective", "Tubelens", "Baseport", "Sideport", "Mirror" };
	std::vector<int> m_maxOptions = { 5, 6, 3, 3, 3, 2 };
	typedef enum enDeviceElement {
		REFLECTOR,
		OBJECTIVE,
		TUBELENS,
		BASEPORT,
		SIDEPORT,
		MIRROR,
		DEVICE_ELEMENT_COUNT
	} DEVICE_ELEMENT;
	std::vector<int> m_availableElements;

	bool getConnectionStatus();

	virtual void setPosition(POINT3 position) = 0;
	// moves the position relative to current position
	void movePosition(POINT3 distance);
	virtual POINT3 getPosition() = 0;

	QTimer *positionTimer = nullptr;

public slots:
	virtual void init() = 0;
	virtual bool connectDevice() = 0;
	virtual bool disconnectDevice() = 0;
	virtual void setElement(ScanControl::DEVICE_ELEMENT, int) = 0;
	virtual void setElements(ScanControl::SCAN_PRESET) = 0;
	virtual void getElements() = 0;
	void announcePosition();
	void startAnnouncingPosition();
	void stopAnnouncingPosition();
	// sets the position relative to the home position m_homePosition
	virtual void setPositionRelativeX(double position) = 0;
	virtual void setPositionRelativeY(double position) = 0;
	virtual void setPositionRelativeZ(double position) = 0;
	void setHome();
	void announceBounds();
	void moveHome();
	void savePosition();
	void moveToSavedPosition(int index);
	void deleteSavedPosition(int index);
	virtual void loadVoltagePositionMap(std::string filepath) {};

	std::vector<POINT3> getSavedPositionsNormalized();
	void announceSavedPositionsNormalized();

signals:
	void connectedDevice(bool);
	void elementPositionsChanged(std::vector<int>);
	void elementPositionChanged(ScanControl::DEVICE_ELEMENT, int);
	void currentPosition(POINT3);
	void savedPositionsChanged(std::vector<POINT3>);
	void boundsChanged(BOUNDS);
};

#endif // SCANCONTROL_H