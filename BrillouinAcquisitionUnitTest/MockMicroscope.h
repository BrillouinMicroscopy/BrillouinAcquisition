#include "..\BrillouinAcquisition\src\Devices\ScanControls\ZeissECU.h"
#include <QObject>

class MockMicroscope : public com {
	Q_OBJECT
private:
	bool m_isOpen = false;
	std::string m_outputBuffer;
public:
	MockMicroscope();
	~MockMicroscope();

	std::string readOutputBuffer();

	qint64 writeToDevice(const char *data);
};