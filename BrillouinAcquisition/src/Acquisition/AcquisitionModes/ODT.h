#ifndef ODT_H
#define ODT_H

#include "AcquisitionMode.h"
#include "..\..\Devices\Cameras\Camera.h"
#include "..\..\Devices\ScanControls\ODTControl.h"
#include "..\..\circularBuffer.h"

enum class ODT_SETTING {
	VOLTAGE,
	NRPOINTS,
	SCANRATE
};

enum class ODT_MODE {
	ALGN,
	ACQ
};

struct ODT_SETTINGS {
	double radialVoltage{ 0.3 };	// [V]	maximum voltage for the galvo scanners
	int numberPoints{ 30 };			// [1]	number of points
	double scanRate{ 1 };			// [Hz]	scan rate, for alignment: rate for one rotation, for acquisition: rate for one step
	std::vector<VOLTAGE2> voltages;	// [V]	voltages to apply
};

class ODT : public AcquisitionMode {
	Q_OBJECT

public:
	ODT(QObject *parent, Acquisition *acquisition, Camera **camera, ODTControl **ODTControl);
	~ODT();

	bool isAlgnRunning();
	void setAlgnSettings(ODT_SETTINGS);
	void setSettings(ODT_SETTINGS);

	bool m_abortAlignment{ false };

public slots:
	void startRepetitions() override;

	void init() override;

	void initialize();
	void startAlignment();
	void centerAlignment();
	void setSettings(ODT_MODE, ODT_SETTING, double);
	void setCameraSetting(CAMERA_SETTING, double);

private:
	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;
	void abortMode();

	void calculateVoltages(ODT_MODE);

	template <typename T>
	void __acquire(std::unique_ptr <StorageWrapper>& storage);

	ODT_SETTINGS m_acqSettings{
		0.3,
		150,
		100,
		{}
	};
	ODT_SETTINGS m_algnSettings;
	CAMERA_SETTINGS m_cameraSettings{ 0.002, 0 };
	Camera** m_camera{ nullptr };
	ODTControl** m_ODTControl{ nullptr };
	bool m_algnRunning{ false };			// is alignment currently running

	int m_algnPositionIndex{ 0 };

	QTimer* m_algnTimer{ nullptr };

private slots:
	void acquire(std::unique_ptr <StorageWrapper> & storage) override;
	
	void nextAlgnPosition();

signals:
	void s_acqSettingsChanged(ODT_SETTINGS);				// emit the acquisition voltages
	void s_algnSettingsChanged(ODT_SETTINGS);				// emit the alignment voltages
	void s_cameraSettingsChanged(CAMERA_SETTINGS);			// emit the camera settings
	void s_mirrorVoltageChanged(VOLTAGE2, ODT_MODE);		// emit the mirror voltage
};

#endif //ODT_H