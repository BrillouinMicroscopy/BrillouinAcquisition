#ifndef BRILLOUINACQUISITON_H
#define BRILLOUINACQUISITON_H

#include <gsl/gsl>

#include "thread.h"

#include "Devices/andor.h"
#include "Devices/pvcamera.h"
#include "Devices/scancontrol.h"
#include "Devices/ZeissECU.h"
#include "Devices/ZeissMTB.h"
#include "Devices/ZeissMTB_Erlangen.h"
#include "Devices/NIDAQ.h"
#include "Devices/PointGrey.h"
#include "Devices/uEyeCam.h"

#include "Acquisition/Acquisition.h"
#include "external/qcustomplot/qcustomplot.h"
#include "external/h5bm/h5bm.h"
#include "tableModel.h"

#include "Acquisition/AcquisitionModes/Brillouin.h"
#include "Acquisition/AcquisitionModes/ODT.h"
#include "Acquisition/AcquisitionModes/Fluorescence.h"
#include "Acquisition/AcquisitionModes/Calibration.h"

#include "converter.h"

#include <QtWidgets/QMainWindow>
#include "ui_BrillouinAcquisition.h"

#include <vector>
#include <string>

typedef struct {

} STAGE_SETTINGS;

typedef struct {
	CAMERA_SETTINGS camera;
	STAGE_SETTINGS stage;
} SETTINGS_DEVICES;

enum ROI_SOURCE {
	BOX,
	PLOT
};

Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(AT_64);
Q_DECLARE_METATYPE(StoragePath);
Q_DECLARE_METATYPE(ACQUISITION_MODE);
Q_DECLARE_METATYPE(ACQUISITION_STATUS);
Q_DECLARE_METATYPE(BRILLOUIN_SETTINGS);
Q_DECLARE_METATYPE(CAMERA_SETTINGS);
Q_DECLARE_METATYPE(CAMERA_SETTING);
Q_DECLARE_METATYPE(CAMERA_OPTIONS);
Q_DECLARE_METATYPE(std::vector<int>);
Q_DECLARE_METATYPE(std::vector<double>);
Q_DECLARE_METATYPE(std::vector<float>);
Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
Q_DECLARE_METATYPE(IMAGE*);
Q_DECLARE_METATYPE(CALIBRATION*);
Q_DECLARE_METATYPE(ScanPreset);
Q_DECLARE_METATYPE(DeviceElement);
Q_DECLARE_METATYPE(SensorTemperature);
Q_DECLARE_METATYPE(POINT3);
Q_DECLARE_METATYPE(POINT2);
Q_DECLARE_METATYPE(std::vector<POINT3>);
Q_DECLARE_METATYPE(BOUNDS);
Q_DECLARE_METATYPE(QMouseEvent*);
Q_DECLARE_METATYPE(VOLTAGE2);
Q_DECLARE_METATYPE(ODT_MODE);
Q_DECLARE_METATYPE(ODT_SETTING);
Q_DECLARE_METATYPE(ODT_SETTINGS);
Q_DECLARE_METATYPE(ODTIMAGE*);
Q_DECLARE_METATYPE(FLUOIMAGE*);
Q_DECLARE_METATYPE(FLUORESCENCE_SETTINGS);
Q_DECLARE_METATYPE(FLUORESCENCE_MODE);
Q_DECLARE_METATYPE(PLOT_SETTINGS*);
Q_DECLARE_METATYPE(PreviewBuffer<unsigned char>*);
Q_DECLARE_METATYPE(unsigned char*);
Q_DECLARE_METATYPE(unsigned short*);
Q_DECLARE_METATYPE(std::vector<unsigned char>);
Q_DECLARE_METATYPE(std::vector<unsigned short>);
Q_DECLARE_METATYPE(bool*);
Q_DECLARE_METATYPE(std::vector<FLUORESCENCE_MODE>);
Q_DECLARE_METATYPE(SpatialCalibration);

class BrillouinAcquisition : public QMainWindow {
	Q_OBJECT

public:
	BrillouinAcquisition(QWidget* parent = nullptr) noexcept;
	~BrillouinAcquisition();

private:
	void initScanControl();
	void initODT();
	void initSpatialCalibration();
	void initFluorescence();
	void initCamera();
	void initCameraBrillouin();

	void checkElementButtons();
	void addListToComboBox(QComboBox*, std::vector<std::wstring>, bool clear = true);

	template <typename T>
	void updateImage(PreviewBuffer<T>* previewBuffer, PLOT_SETTINGS* plotSettings);

	template<typename T>
	void plotting(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, std::vector<T> unpackedBuffer);

	Ui::BrillouinAcquisitionClass* ui;
	ScanControl::SCAN_DEVICE m_scanControllerType = ScanControl::SCAN_DEVICE::ZEISSECU;
	ScanControl::SCAN_DEVICE m_scanControllerTypeTemporary = m_scanControllerType;

	typedef enum class enCameraDevice {
		NONE = 0,
		POINTGREY = 1,
		UEYE = 2
	} CAMERA_DEVICE;
	std::vector<std::string> CAMERA_DEVICE_NAMES = { "None", "PointGrey", "uEye" };

	typedef enum class enCameraBrillouinDevice {
		ANDOR = 0,
		PVCAM = 1
	} CAMERA_BRILLOUIN_DEVICE;
	std::vector<std::string> CAMERA_BRILLOUIN_DEVICE_NAMES = { "Andor", "PVCam" };

	QCPGraph* m_focusMarker{ nullptr };
	POINT2 m_focusMarkerPos{ -1, -1 };
	bool m_selectFocus{ false };

	CAMERA_DEVICE m_cameraType{ CAMERA_DEVICE::UEYE };
	CAMERA_DEVICE m_cameraTypeTemporary = m_cameraType;

	CAMERA_BRILLOUIN_DEVICE m_cameraBrillouinType = CAMERA_BRILLOUIN_DEVICE::PVCAM;
	CAMERA_BRILLOUIN_DEVICE m_cameraBrillouinTypeTemporary = m_cameraBrillouinType;
	QComboBox* m_scanControlDropdown;
	QComboBox* m_cameraDropdown;
	QComboBox* m_cameraBrillouinDropdown;
	std::string m_calibrationFilePath;

	QDialog* m_settingsDialog{ nullptr };
	Camera* m_andor{ nullptr };
	ScanControl* m_scanControl{ nullptr };
	Camera* m_brightfieldCamera{ nullptr };
	Acquisition* m_acquisition = new Acquisition(nullptr);

	// Threads
	Thread m_andorThread;
	Thread m_brightfieldCameraThread;
	Thread m_acquisitionThread;
	Thread m_plottingThread;

	Brillouin* m_Brillouin = new Brillouin(nullptr, m_acquisition, &m_andor, &m_scanControl);
	BRILLOUIN_SETTINGS m_BrillouinSettings;
	ODT* m_ODT{ nullptr };
	Fluorescence* m_Fluorescence{ nullptr };
	Calibration* m_Calibration{ nullptr };

	PLOT_SETTINGS m_BrillouinPlot;
	PLOT_SETTINGS m_ODTPlot;

	converter* m_converter = new converter();

	SETTINGS_DEVICES m_deviceSettings;
	CAMERA_OPTIONS m_cameraOptions;
	CAMERA_OPTIONS m_cameraOptionsODT;
	StoragePath m_storagePath{ "", "." };
	bool m_previewRunning{ false };
	bool m_brightfieldPreviewRunning{ false };
	ACQUISITION_MODE m_enabledModes{ ACQUISITION_MODE::NONE };

	bool m_hasODT{ false };
	bool m_hasSpatialCalibration{ false };
	bool m_isTabVisibleODT{ false };
	bool m_hasFluorescence{ false };
	bool m_isTabVisibleFluorescence{ false };

	TableModel* tableModel = new TableModel(0);
	ButtonDelegate buttonDelegate;

	std::vector<double> m_deviceElementPositions = { 0, 0, 0, 0, 0, 0 };

	std::vector<std::vector<QPushButton*>> elementButtons;
	std::vector<QSpinBox*> elementIntBox;
	std::vector<QDoubleSpinBox*> elementDoubleBox;
	std::vector<QSlider*> elementSlider;
	std::vector<QPushButton*> presetButtons;

	struct {
		QIcon disconnected;
		QIcon standby;
		QIcon cooling;
		QIcon ready;
		QIcon fluoBlue;
		QIcon fluoGreen;
		QIcon fluoRed;
		QIcon fluoBrightfield;
	} m_icons;
	
private slots:
	void on_rangeLower_valueChanged(int);
	void on_rangeUpper_valueChanged(int);
	void on_rangeLowerODT_valueChanged(int);
	void on_rangeUpperODT_valueChanged(int);
	void updatePlot(PLOT_SETTINGS plotSettings);
	void updateCLimRange(QSpinBox*, QSpinBox*, QCPRange);

	void on_addFocusMarker_brightfield_clicked();

	void showEvent(QShowEvent* event);
	void on_actionAbout_triggered();
	void on_camera_singleShot_clicked();
	// connect camera and react
	void on_actionConnect_Camera_triggered();
	void cameraConnectionChanged(bool);
	void showNoCameraFound();
	// enable camera cooling and react
	void on_actionEnable_Cooling_triggered();
	void cameraCoolingChanged(bool);
	// connect microscope and react
	void on_actionConnect_Stage_triggered();
	void microscopeConnectionChanged(bool);

	void on_actionConnect_Brightfield_camera_triggered();
	void brightfieldCameraConnectionChanged(bool);
	void on_camera_playPause_brightfield_clicked();

	void on_actionSettings_Stage_triggered();
	void saveSettings();
	void cancelSettings();
	void initSettingsDialog();
	void selectScanningDevice(int index);
	void selectCameraDevice(int index);
	void selectCameraBrillouinDevice(int index);

	void on_actionAcquire_Voltage_Position_calibration_triggered();
	void on_actionLoad_Voltage_Position_calibration_triggered();

	void on_microscopeWidth_valueChanged(int);
	void on_microscopeHeight_valueChanged(int);
	void on_microscopeMag_valueChanged(double);
	void on_microscopePixSize_valueChanged(double);

	void initBeampathButtons();

	void on_BrillouinStart_clicked();
	void microscopeElementPositionsChanged(std::vector<double>);
	void microscopeElementPositionChanged(DeviceElement element, double position);
	void on_camera_playPause_clicked();

	void updateImageBrillouin();
	void updateImageODT();

	void plot(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<unsigned char> unpackedBuffer);
	void plot(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<unsigned short> unpackedBuffer);
	void plot(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<double> unpackedBuffer);
	void plot(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<float> unpackedBuffer);

	void initializePlot(PLOT_SETTINGS plotSettings);

	void drawFocusMarker();

	void xAxisRangeChangedODT(const QCPRange& newRange);
	void yAxisRangeChangedODT(const QCPRange& newRange);

	void plotClick(QMouseEvent* event);

	// set and check camera ROI
	void xAxisRangeChanged(const QCPRange& newRange);
	void yAxisRangeChanged(const QCPRange& newRange);
	void on_ROILeft_valueChanged(int);
	void on_ROIWidth_valueChanged(int);
	void on_ROITop_valueChanged(int);
	void on_ROIHeight_valueChanged(int);
	void settingsCameraUpdate(int);
	std::vector<AT_64> checkROI(std::vector<AT_64>, std::vector<AT_64>);

	/*
	 * Camera settings
	 */
	// Binning
	void on_binning_currentIndexChanged(const QString& text);
	// Readout parameters
	void on_pixelReadoutRate_currentIndexChanged(const QString& text);
	void on_preAmpGain_currentIndexChanged(const QString& text);
	void on_pixelEncoding_currentIndexChanged(const QString& text);
	void on_cycleMode_currentIndexChanged(const QString& text);

	void applyCameraSettings();

	void setColormap(QCPColorGradient*, CustomGradientPreset);
	void setElement(DeviceElement element, double position);
	void setPreset(ScanPreset preset);
	void updatePlotLimits(PLOT_SETTINGS plotSettings, CAMERA_OPTIONS options, CAMERA_ROI roi);
	void showPreviewRunning(bool);
	void showBrightfieldPreviewRunning(bool isRunning);
	void showFluorescencePreviewRunning(FLUORESCENCE_MODE mode);
	void startPreview(bool);
	void startBrightfieldPreview(bool isRunning);
	void cameraSettingsChanged(CAMERA_SETTINGS);
	void cameraODTSettingsChanged(CAMERA_SETTINGS settings);
	void updateODTCameraSettings(CAMERA_SETTINGS settings);
	void sensorTemperatureChanged(SensorTemperature);
	void initializeODTVoltagePlot(QCustomPlot* plot);
	void plotODTVoltages(ODT_SETTINGS settings, ODT_MODE mode);
	void plotODTVoltage(VOLTAGE2 voltage, ODT_MODE mode);
	void cameraOptionsChanged(CAMERA_OPTIONS);
	void cameraODTOptionsChanged(CAMERA_OPTIONS options);
	void showAcqPosition(POINT3, int);
	void showPosition(POINT3);
	void setHomePositionBounds(BOUNDS);
	void setCurrentPositionBounds(BOUNDS bounds);
	void showCalibrationInterval(int);
	void showCalibrationRunning(bool);
	void updateFilename(std::string);
	void updateCalibration(SpatialCalibration);

	void showEnabledModes(ACQUISITION_MODE mode);
	void showBrillouinStatus(ACQUISITION_STATUS state);
	void showBrillouinProgress(double progress, int seconds);
	void showODTStatus(ACQUISITION_STATUS state);
	void showODTProgress(double progress, int seconds);
	void showFluorescenceStatus(ACQUISITION_STATUS state);
	void showFluorescenceProgress(double progress, int seconds);

	// ODT signals
	void on_alignmentUR_ODT_valueChanged(double);
	void on_alignmentNumber_ODT_valueChanged(int);
	void on_alignmentRate_ODT_valueChanged(double);
	void on_alignmentStartODT_clicked();
	void on_alignmentCenterODT_clicked();
	void on_acquisitionUR_ODT_valueChanged(double);
	void on_acquisitionNumber_ODT_valueChanged(int);
	void on_acquisitionRate_ODT_valueChanged(double);
	void on_acquisitionStartODT_clicked();
	void on_exposureTimeODT_valueChanged(double);
	void on_gainODT_valueChanged(double);

	void on_exposureTimeCameraODT_valueChanged(double exposureTime);
	void on_gainCameraODT_valueChanged(double gain);

	void on_camera_displayMode_currentIndexChanged(const QString &text);
	void on_setBackground_clicked();

	void applyGradient(PLOT_SETTINGS plotSettings);

	/*
	 * Fluorescence slots
	 */
	// Acquire slots
	void on_acquisitionStartFluorescence_clicked();	// Acquire all enabled channels
	void on_fluoBlueStart_clicked();				// Acquire the blue channel
	void on_fluoGreenStart_clicked();				// Acquire the green channel
	void on_fluoRedStart_clicked();					// Acquire the red channel
	void on_fluoBrightfieldStart_clicked();			// Acquire the brightfield channel
	// Preview slots
	void on_fluoBluePreview_clicked();
	void on_fluoGreenPreview_clicked();
	void on_fluoRedPreview_clicked();
	void on_fluoBrightfieldPreview_clicked();
	// Enable channel slots
	void on_fluoBlueCheckbox_stateChanged(int);
	void on_fluoGreenCheckbox_stateChanged(int);
	void on_fluoRedCheckbox_stateChanged(int);
	void on_fluoBrightfieldCheckbox_stateChanged(int);
	// Set exposure time slots
	void on_fluoBlueExposure_valueChanged(int);
	void on_fluoGreenExposure_valueChanged(int);
	void on_fluoRedExposure_valueChanged(int);
	void on_fluoBrightfieldExposure_valueChanged(int);
	// Set camera gain slots
	void on_fluoBlueGain_valueChanged(double gain);
	void on_fluoGreenGain_valueChanged(double gain);
	void on_fluoRedGain_valueChanged(double gain);
	void on_fluoBrightfieldGain_valueChanged(double gain);
	// Settings changed slot
	void updateFluorescenceSettings(FLUORESCENCE_SETTINGS settings);


	QString formatSeconds(int seconds);

	void on_actionQuit_triggered();

	void on_autoscalePlot_stateChanged(int);
	void on_autoscalePlot_brightfield_stateChanged(int);

	void updateBrillouinSettings();

	void on_exposureTime_valueChanged(double);
	void on_frameCount_valueChanged(int);

	StoragePath splitFilePath(QString fullPath);
	QString checkFilename(QString absoluteFilePath);

	void on_actionNew_Acquisition_triggered();
	void on_actionOpen_Acquisition_triggered();
	void on_actionClose_Acquisition_triggered();

	// acquisition AOI
	void on_startX_valueChanged(double);
	void on_startY_valueChanged(double);
	void on_startZ_valueChanged(double);
	void on_endX_valueChanged(double);
	void on_endY_valueChanged(double);
	void on_endZ_valueChanged(double);
	void on_stepsX_valueChanged(int);
	void on_stepsY_valueChanged(int);
	void on_stepsZ_valueChanged(int);

	// live calibration
	void on_preCalibration_stateChanged(int);
	void on_postCalibration_stateChanged(int);
	void on_conCalibration_stateChanged(int);
	void on_sampleSelection_currentIndexChanged(const QString &text);
	void on_conCalibrationInterval_valueChanged(double);
	void on_nrCalibrationImages_valueChanged(int);
	void on_calibrationExposureTime_valueChanged(double);

	// repetitions
	void on_repetitionCount_valueChanged(int);
	void on_repetitionInterval_valueChanged(double);
	void showRepProgress(int repNumber, int timeToNext);

	// manual stage control
	void on_savePosition_clicked();
	void on_setHome_clicked();
	void on_moveHome_clicked();

	void on_setPositionX_valueChanged(double);
	void on_setPositionY_valueChanged(double);
	void on_setPositionZ_valueChanged(double);
	void updateSavedPositions();

	/*
	 *	Scan direction order related functions
	 */
	void on_scanDirAutoCheckbox_stateChanged(int);
	void on_buttonGroup_buttonClicked(int);
	void on_buttonGroup_2_buttonClicked(int);
	void on_buttonGroup_3_buttonClicked(int);

	void scanOrderChanged(SCAN_ORDER scanOrder);

	/*
	 * Save and restore application settings
	 */
	void writeSettings();
	void readSettings();
};

#endif // BRILLOUINACQUISITON_H