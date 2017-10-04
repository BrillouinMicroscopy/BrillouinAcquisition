#include "stdafx.h"
#include "thread.h"

void Thread::startWorker(QObject *worker) {
	start();
	worker->moveToThread(this);
}
