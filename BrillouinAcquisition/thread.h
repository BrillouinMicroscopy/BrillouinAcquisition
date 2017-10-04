#ifndef THREAD_H
#define THREAD_H

#include <QtCore>

class Thread :public QThread {
	Q_OBJECT

public:
	void startWorker(QObject *worker);

private:
	QObject *worker;
};

#endif // THREAD_H
