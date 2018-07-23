#ifndef THREAD_H
#define THREAD_H

#include <QtCore>

class Thread :public QThread {
	Q_OBJECT

public:
	void startWorker(QObject *worker) {
		worker->moveToThread(this);
		if (!this->isRunning()) {
			this->start();
		}

		QMetaObject::invokeMethod(worker, "init", Qt::AutoConnection);
	}
};

#endif // THREAD_H
