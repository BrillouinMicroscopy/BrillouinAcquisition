#ifndef PREVIEWBUFFER_H
#define PREVIEWBUFFER_H

#include <QtCore>
#include <gsl/gsl>
#include "atcore.h"
#include "circularBuffer.h"
#include "cameraParameters.h"

struct BUFFER_SETTINGS {
	int bufferNumber = 0;
	int bufferSize = 0;
	CAMERA_ROI roi;
	BUFFER_SETTINGS() noexcept {};
	BUFFER_SETTINGS(int bufferNumber, int bufferSize, CAMERA_ROI roi) : roi(roi), bufferNumber(bufferNumber), bufferSize(bufferSize) {};
};

template<class T> class PreviewBuffer {

public:
	PreviewBuffer() noexcept;
	PreviewBuffer(BUFFER_SETTINGS bufferSettings);
	~PreviewBuffer();

	void initializeBuffer(BUFFER_SETTINGS bufferSettings);

	CircularBuffer<T>* m_buffer = new CircularBuffer<T>;
	BUFFER_SETTINGS m_bufferSettings;
};

template<class T>
inline PreviewBuffer<T>::PreviewBuffer() noexcept {
}

template<class T>
inline PreviewBuffer<T>::PreviewBuffer(BUFFER_SETTINGS bufferSettings) :
	m_bufferSettings(bufferSettings) {
	
}

template<class T>
inline PreviewBuffer<T>::~PreviewBuffer() {
	delete m_buffer;
}

template<class T>
void inline PreviewBuffer<T>::initializeBuffer(BUFFER_SETTINGS bufferSettings) {
	m_bufferSettings = bufferSettings;
	if (m_buffer != nullptr) {
		delete m_buffer;
	}
	m_buffer = new CircularBuffer<T>(m_bufferSettings.bufferNumber, m_bufferSettings.bufferSize);
}

#endif //PREVIEWBUFFER_H