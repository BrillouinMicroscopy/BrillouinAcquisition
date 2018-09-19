#include "../BrillouinAcquisition/src/ZeissECU.h"
#include <QObject>

class MockMicroscope : public com {
	Q_OBJECT
private:
	bool m_isOpen = false;
	OpenMode m_mode;
	std::string m_outputBuffer;
public:
	MockMicroscope();
	~MockMicroscope();

	std::string readOutputBuffer();

	qint64 writeToDevice(const char *data);
protected:
	bool open(OpenMode mode) Q_DECL_OVERRIDE;
	void close() Q_DECL_OVERRIDE;
};