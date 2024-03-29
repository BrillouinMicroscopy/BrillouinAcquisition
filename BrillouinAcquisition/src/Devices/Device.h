#ifndef DEVICE_H
#define DEVICE_H

#include <QtCore>
#include <gsl/gsl>
#include "../lib/math/simplemath.h"

class Device : public QObject {
	Q_OBJECT

public:
	Device() {};
	~Device() {};

	bool getConnectionStatus();

public slots:
	virtual void init() = 0;
	virtual void connectDevice() = 0;
	virtual void disconnectDevice() = 0;

protected:
	bool m_isConnected{ false };

signals:
	void connectedDevice(bool);
};

#endif //DEVICE_H