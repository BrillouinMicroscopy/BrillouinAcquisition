#include "stdafx.h"
#include "storageWrapper.h"
#include "logger.h"


StorageWrapper::~StorageWrapper() {
	// clear image queue in case acquisition was aborted
	// and the queue is still filled
	while (!m_payloadQueue.isEmpty()) {
		IMAGE *img = m_payloadQueue.dequeue();
		delete img;
	}
	while (!m_calibrationQueue.isEmpty()) {
		CALIBRATION *cal = m_calibrationQueue.dequeue();
		delete cal;
	}
}

void StorageWrapper::startWritingQueues() {
	m_observeQueues = TRUE;
	s_writeQueues();
}

void StorageWrapper::stopWritingQueues() {
	m_observeQueues = FALSE;
}

void StorageWrapper::s_writeQueues() {
	while (!m_payloadQueue.isEmpty()) {
		if (m_abort) {
			m_finishedWriting = TRUE;
			return;
		}
		IMAGE *img = m_payloadQueue.dequeue();
		setPayloadData(img->indX, img->indY, img->indZ, img->data, img->rank, img->dims, img->date);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
	}

	while (!m_calibrationQueue.isEmpty()) {
		if (m_abort) {
			m_finishedWriting = TRUE;
			return;
		}
		CALIBRATION *cal = m_calibrationQueue.dequeue();
		setCalibrationData(cal->index, cal->data, cal->rank, cal->dims, cal->sample, cal->shift, cal->date);
		//std::string info = "Calibration written " + std::to_string(m_writtenCalibrationsNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenCalibrationsNr++;
		delete cal;
	}

	if (m_observeQueues && !m_abort) {
		QMetaObject::invokeMethod(this, "s_writeQueues", Qt::QueuedConnection);
	} else {
		m_finishedWriting = TRUE;
	}
}