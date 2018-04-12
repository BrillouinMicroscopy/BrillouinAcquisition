#include "../BrillouinAcquisition/scancontrol.h"
#include <QObject>

class MockMicroscope : public com {
	Q_OBJECT
private:
	bool m_isOpen = false;
	OpenMode m_mode;
	QByteArray m_outputBuffer;
public:
	MockMicroscope();
	~MockMicroscope();
protected:
	//qint64 readData(char* data, qint64 maxlen) Q_DECL_OVERRIDE;
	//qint64 writeData(const char* data, qint64 len) Q_DECL_OVERRIDE;

	bool open(OpenMode mode) Q_DECL_OVERRIDE;
	void close() Q_DECL_OVERRIDE;

	qint64 readCharacter(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
	qint64 writeData(const char *data, qint64 maxSize);

	bool waitForBytesWritten(int msecs = 30000);

	bool waitForReadyRead(int msecs = 30000);
};