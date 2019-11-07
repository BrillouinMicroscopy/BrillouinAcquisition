#ifndef FLUORESCENCE_H
#define FLUORESCENCE_H

#include "AcquisitionMode.h"
#include "../../Devices/PointGrey.h"
#include "../../Devices/scancontrol.h"

enum class FLUORESCENCE_MODE {
	NONE,
	BLUE,
	GREEN,
	RED,
	BRIGHTFIELD,
	MODE_COUNT
};

struct ChannelSettings {
	bool enabled{ true };
	int exposure{ 900 };
	int gain{ 10 };
	std::string name;
	FLUORESCENCE_MODE mode;
	ScanPreset preset;
};

struct FLUORESCENCE_SETTINGS {
	ChannelSettings blue{ true, 900, 10, "blue", FLUORESCENCE_MODE::BLUE, ScanPreset::SCAN_EPIFLUOBLUE };
	ChannelSettings green{ true, 900, 10, "Green", FLUORESCENCE_MODE::GREEN, ScanPreset::SCAN_EPIFLUOGREEN };
	ChannelSettings red{ true, 900, 10, "Red", FLUORESCENCE_MODE::RED, ScanPreset::SCAN_EPIFLUORED };
	ChannelSettings brightfield{ true, 4, 0, "Brightfield", FLUORESCENCE_MODE::BRIGHTFIELD, ScanPreset::SCAN_BRIGHTFIELD };
	CAMERA_SETTINGS camera;
};

class Fluorescence : public AcquisitionMode {
	Q_OBJECT

public:
	Fluorescence(QObject *parent, Acquisition *acquisition, Camera **camera, ScanControl **scanControl);
	~Fluorescence();

public slots:
	void startRepetitions() override;
	void startRepetitions(std::vector<FLUORESCENCE_MODE> modes);

	void initialize();
	void setChannel(FLUORESCENCE_MODE, bool);
	void setExposure(FLUORESCENCE_MODE, int);
	void setGain(FLUORESCENCE_MODE mode, int gain);
	void startStopPreview(FLUORESCENCE_MODE);

private:
	void abortMode(std::unique_ptr <StorageWrapper>& storage) override;

	ChannelSettings* getChannelSettings(FLUORESCENCE_MODE mode);
	std::vector<ChannelSettings*> getEnabledChannels();
	void configureCamera();

	Camera** m_camera;
	ScanControl** m_scanControl;

	FLUORESCENCE_SETTINGS m_settings;
	FLUORESCENCE_MODE previewChannel{ FLUORESCENCE_MODE::NONE };


private slots:
	void acquire(std::unique_ptr <StorageWrapper>& storage) override;
	void acquire(std::unique_ptr <StorageWrapper>& storage, std::vector<ChannelSettings *> channels);

signals:
	void s_acqSettingsChanged(FLUORESCENCE_SETTINGS);
	void s_previewRunning(FLUORESCENCE_MODE);
};

#endif //FLUORESCENCE_H