#ifndef FILTERMOUNT_H
#define FILTERMOUNT_H

#include "Device.h"
#include "com.h"

class FilterMount: public Device {
	Q_OBJECT

private:
	com * m_comObject = nullptr;
	QString m_comPort;

	std::string parsePosition(std::string position);

public:
	FilterMount(QString comPort) : m_comPort(comPort) {};
	~FilterMount();

	void home();

	double getPosition();
	void setPosition(double);

	void moveForward();
	void moveBackward();

public slots:
	void init();
	void connectDevice();
	void disconnectDevice();
};

#endif // FILTERMOUNT_H