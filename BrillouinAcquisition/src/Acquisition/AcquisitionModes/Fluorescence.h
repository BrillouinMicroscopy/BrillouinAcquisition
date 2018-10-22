#ifndef FLUORESCENCE_H
#define FLUORESCENCE_H

#include "AcquisitionMode.h"
#include "../../Devices/PointGrey.h"
#include "../../Devices/NIDAQ.h"

enum class FLUORESCENCE_MODE {
	BLUE,
	GREEN,
	RED,
	BRIGHTFIELD,
	MODE_COUNT
};

struct ChannelSettings {
	bool enabled{ true };
	int exposure{ 900 };
	std::string name;
	FLUORESCENCE_MODE mode;
	SCAN_PRESET preset;
};

struct FLUORESCENCE_SETTINGS {
	ChannelSettings blue{ true, 900, "blue", FLUORESCENCE_MODE::BLUE, SCAN_EPIFLUOBLUE };
	ChannelSettings green{ true, 900, "Green", FLUORESCENCE_MODE::GREEN, SCAN_EPIFLUOGREEN };
	ChannelSettings red{ true, 900, "Red", FLUORESCENCE_MODE::RED, SCAN_EPIFLUORED };
	ChannelSettings brightfield{ true, 900, "Brightfield", FLUORESCENCE_MODE::BRIGHTFIELD, SCAN_BRIGHTFIELD };
	CAMERA_SETTINGS camera;
};

class Fluorescence : public AcquisitionMode {
	Q_OBJECT

public:
	Fluorescence(QObject *parent, Acquisition *acquisition, PointGrey **pointGrey, NIDAQ **nidaq);
	~Fluorescence();

public slots:
	void init() {};
	void initialize();
	void startRepetitions();
	void setGain(int);
	void setChannel(FLUORESCENCE_MODE, bool);
	void setExposure(FLUORESCENCE_MODE, int);

private:
	PointGrey **m_pointGrey;
	NIDAQ **m_NIDAQ;

	FLUORESCENCE_SETTINGS m_settings;
	ChannelSettings * getChannelSettings(FLUORESCENCE_MODE mode);
	std::vector<ChannelSettings *> getEnabledChannels();

	void abortMode() override;

private slots:
	void acquire(std::unique_ptr <StorageWrapper> & storage) override;

signals:
	void s_acqSettingsChanged(FLUORESCENCE_SETTINGS);
};

#endif //FLUORESCENCE_H