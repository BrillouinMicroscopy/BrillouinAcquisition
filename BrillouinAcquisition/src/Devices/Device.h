#ifndef DEVICE_H
#define DEVICE_H

#include <QtCore>
#include <gsl/gsl>

class Device : public QObject {
	Q_OBJECT

public:
	Device();
	~Device();

public slots:
	virtual void init() = 0;
	virtual bool connectDevice() = 0;
	virtual bool disconnectDevice() = 0;
};

#endif //DEVICE_H