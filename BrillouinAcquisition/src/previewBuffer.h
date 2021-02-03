#ifndef PREVIEWBUFFER_H
#define PREVIEWBUFFER_H

#include <QtCore>
#include <gsl/gsl>
#include "circularBuffer.h"
#include "Devices\Cameras\cameraParameters.h"

struct BUFFER_SETTINGS {
	int bufferNumber{ 0 };
	unsigned int bufferSize{ 0 };
	std::string bufferType{ "unsigned char" };
	CAMERA_ROI roi;
	BUFFER_SETTINGS() noexcept {};
	BUFFER_SETTINGS(int bufferNumber, unsigned int bufferSize, std::string bufferType, CAMERA_ROI roi) : roi(roi), bufferNumber(bufferNumber),
		bufferSize(bufferSize), bufferType(bufferType) {};
};

template<class T> class PreviewBuffer {

public:
	PreviewBuffer() noexcept;
	explicit PreviewBuffer(BUFFER_SETTINGS bufferSettings);
	~PreviewBuffer();

	void initializeBuffer(BUFFER_SETTINGS bufferSettings);

	std::mutex m_mutex;

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
	if (m_buffer) {
		delete m_buffer;
		m_buffer = nullptr;
	}
}

template<class T>
void inline PreviewBuffer<T>::initializeBuffer(BUFFER_SETTINGS bufferSettings) {
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	m_bufferSettings = bufferSettings;
	if (m_buffer != nullptr) {
		delete m_buffer;
		m_buffer = nullptr;
	}
	m_buffer = new CircularBuffer<T>(m_bufferSettings.bufferNumber, m_bufferSettings.bufferSize);
}

#endif //PREVIEWBUFFER_H