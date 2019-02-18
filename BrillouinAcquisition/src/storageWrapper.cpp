#include "stdafx.h"
#include "storageWrapper.h"
#include "logger.h"


StorageWrapper::~StorageWrapper() {
	// clear image queue in case acquisition was aborted
	// and the queue is still filled
	if (m_abort) {
		while (!m_payloadQueueBrillouin.isEmpty()) {
			IMAGE *img = m_payloadQueueBrillouin.dequeue();
			delete img;
		}
		while (!m_payloadQueueODT.isEmpty()) {
			ODTIMAGE *img = m_payloadQueueODT.dequeue();
			delete img;
		}
		while (!m_payloadQueueFluorescence.isEmpty()) {
			FLUOIMAGE *img = m_payloadQueueFluorescence.dequeue();
			delete img;
		}
		while (!m_calibrationQueue.isEmpty()) {
			CALIBRATION *cal = m_calibrationQueue.dequeue();
			delete cal;
		}
	}
	queueTimer->stop();
	delete queueTimer;
}

void StorageWrapper::init() {
	queueTimer = new QTimer();
	QWidget::connect(queueTimer, SIGNAL(timeout()), this, SLOT(s_writeQueues()));
}

void StorageWrapper::s_enqueuePayload(IMAGE *img) {
	m_payloadQueueBrillouin.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(ODTIMAGE *img) {
	m_payloadQueueODT.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(FLUOIMAGE *img) {
	m_payloadQueueFluorescence.enqueue(img);
}

void StorageWrapper::s_enqueueCalibration(CALIBRATION *cal) {
	m_calibrationQueue.enqueue(cal);
}

void StorageWrapper::s_finishedQueueing() {
	m_finishedQueueing = true;
}

void StorageWrapper::startWritingQueues() {
	m_observeQueues = true;
	m_finishedQueueing = false;
	emit(started());
	queueTimer->start(50);
}

void StorageWrapper::stopWritingQueues() {
	m_observeQueues = false;
	queueTimer->stop();
}

void StorageWrapper::s_writeQueues() {
	while (!m_payloadQueueBrillouin.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		IMAGE *img = m_payloadQueueBrillouin.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_payloadQueueODT.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		ODTIMAGE *img = m_payloadQueueODT.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_payloadQueueFluorescence.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		FLUOIMAGE *img = m_payloadQueueFluorescence.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_calibrationQueue.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		CALIBRATION *cal = m_calibrationQueue.dequeue();
		setCalibrationData(cal->index, cal->data, cal->rank, cal->dims, cal->sample, cal->shift, cal->date);
		//std::string info = "Calibration written " + std::to_string(m_writtenCalibrationsNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenCalibrationsNr++;
		delete cal;
		cal = nullptr;
	}

	if (m_finishedQueueing) {
		queueTimer->stop();
		emit(finished());
	}
}