#include "stdafx.h"
#include "storageWrapper.h"
#include "logger.h"


StorageWrapper::~StorageWrapper() {
	// clear image queue in case acquisition was aborted
	// and the queue is still filled
	if (m_abort) {
		while (!m_payloadQueueBrillouin_char.isEmpty()) {
			auto img = m_payloadQueueBrillouin_char.dequeue();
			delete img;
		}
		while (!m_payloadQueueBrillouin_short.isEmpty()) {
			auto img = m_payloadQueueBrillouin_short.dequeue();
			delete img;
		}
		while (!m_payloadQueueODT_char.isEmpty()) {
			auto img = m_payloadQueueODT_char.dequeue();
			delete img;
		}
		while (!m_payloadQueueODT_short.isEmpty()) {
			auto img = m_payloadQueueODT_short.dequeue();
			delete img;
		}
		while (!m_payloadQueueFluorescence_char.isEmpty()) {
			auto img = m_payloadQueueFluorescence_char.dequeue();
			delete img;
		}
		while (!m_payloadQueueFluorescence_short.isEmpty()) {
			auto img = m_payloadQueueFluorescence_short.dequeue();
			delete img;
		}
		while (!m_calibrationQueue_char.isEmpty()) {
			auto cal = m_calibrationQueue_char.dequeue();
			delete cal;
		}
		while (!m_calibrationQueue_short.isEmpty()) {
			auto cal = m_calibrationQueue_short.dequeue();
			delete cal;
		}
		while (!m_calibrationQueue_int.isEmpty()) {
			auto cal = m_calibrationQueue_int.dequeue();
			delete cal;
		}
	}
	queueTimer->stop();
	delete queueTimer;
	emit(finished());
}

void StorageWrapper::init() {
	queueTimer = new QTimer();
	QMetaObject::Connection connection = QWidget::connect(
		queueTimer,
		&QTimer::timeout,
		this,
		&StorageWrapper::s_writeQueues
	);
}

void StorageWrapper::s_enqueuePayload(IMAGE<unsigned char> *img) {
	m_payloadQueueBrillouin_char.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(IMAGE<unsigned short>* img) {
	m_payloadQueueBrillouin_short.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(IMAGE<unsigned int>* img) {
	m_payloadQueueBrillouin_int.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(ODTIMAGE<unsigned char>*img) {
	m_payloadQueueODT_char.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(ODTIMAGE<unsigned short>* img) {
	m_payloadQueueODT_short.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(FLUOIMAGE<unsigned char>*img) {
	m_payloadQueueFluorescence_char.enqueue(img);
}

void StorageWrapper::s_enqueuePayload(FLUOIMAGE<unsigned short>* img) {
	m_payloadQueueFluorescence_short.enqueue(img);
}

void StorageWrapper::s_enqueueCalibration(CALIBRATION<unsigned char>*cal) {
	m_calibrationQueue_char.enqueue(cal);
}

void StorageWrapper::s_enqueueCalibration(CALIBRATION<unsigned short>* cal) {
	m_calibrationQueue_short.enqueue(cal);
}

void StorageWrapper::s_enqueueCalibration(CALIBRATION<unsigned int>* cal) {
	m_calibrationQueue_int.enqueue(cal);
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
	m_finished = true;
	queueTimer->stop();
	m_finishedQueueing = true;
	emit(finished());
}

void StorageWrapper::s_writeQueues() {
	while (!m_payloadQueueBrillouin_char.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueBrillouin_char.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}
	while (!m_payloadQueueBrillouin_short.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueBrillouin_short.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}
	while (!m_payloadQueueBrillouin_int.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueBrillouin_int.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_payloadQueueODT_char.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueODT_char.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}
	while (!m_payloadQueueODT_short.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueODT_short.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_payloadQueueFluorescence_char.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueFluorescence_char.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}
	while (!m_payloadQueueFluorescence_short.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto img = m_payloadQueueFluorescence_short.dequeue();
		setPayloadData(img);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_calibrationQueue_char.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto cal = m_calibrationQueue_char.dequeue();
		setCalibrationData(cal->index, cal->data, cal->rank, cal->dims, cal->sample, cal->shift, cal->date, cal->exposure, cal->gain, cal->binning);
		//std::string info = "Calibration written " + std::to_string(m_writtenCalibrationsNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenCalibrationsNr++;
		delete cal;
		cal = nullptr;
	}

	while (!m_calibrationQueue_short.isEmpty()) {
		if (m_abort) {
			stopWritingQueues();
			return;
		}
		auto cal = m_calibrationQueue_short.dequeue();
		setCalibrationData(cal->index, cal->data, cal->rank, cal->dims, cal->sample, cal->shift, cal->date, cal->exposure, cal->gain, cal->binning);
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