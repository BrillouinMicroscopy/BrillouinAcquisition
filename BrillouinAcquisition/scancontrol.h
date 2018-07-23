#ifndef SCANCONTROL_H
#define SCANCONTROL_H

class ScanControl: public QObject {
	Q_OBJECT

protected:
	bool m_isConnected = false;
	bool m_isCompatible = false;

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

	virtual void setPosition(std::vector<double> position) = 0;
	virtual void setPositionRelative(std::vector<double> distance) = 0;
	virtual std::vector<double> getPosition() = 0;

public slots:
	virtual void init() = 0;
	virtual bool connectDevice() = 0;
	virtual bool disconnectDevice() = 0;
	virtual void setElement(ScanControl::DEVICE_ELEMENT, int) = 0;
	virtual void setElements(ScanControl::SCAN_PRESET) = 0;
	virtual void getElements() = 0;

signals:
	void connectedDevice(bool);
	void elementPositionsChanged(std::vector<int>);
	void elementPositionChanged(ScanControl::DEVICE_ELEMENT, int);
};

#endif // SCANCONTROL_H