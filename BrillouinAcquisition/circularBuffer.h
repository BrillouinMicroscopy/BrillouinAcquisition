#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QtCore>
#include <gsl/gsl>

template<class T> class CircularBuffer {

public:
	CircularBuffer() noexcept;
	CircularBuffer(const int bufferNumber, const int bufferSize);
	~CircularBuffer();

	T* getWriteBuffer();
	T* getReadBuffer();

	QSemaphore* m_freeBuffers;
	QSemaphore* m_usedBuffers = new QSemaphore;

	T** m_buffers;

private:
	static int checkBufferNumber(int bufferNumber);
	const int m_bufferSize;
	const int m_bufferNumber;
	unsigned int m_writeCount = 0;
	unsigned int m_readCount = 0;
};

template<class T>
inline CircularBuffer<T>::CircularBuffer() noexcept : m_bufferNumber(0), m_bufferSize(0), m_freeBuffers(new QSemaphore(0)) {
	m_buffers = nullptr;
}

template<class T>
inline CircularBuffer<T>::CircularBuffer(const int bufferNumber, const int bufferSize) : m_bufferNumber(checkBufferNumber(bufferNumber)), m_bufferSize(bufferSize),
	m_freeBuffers(new QSemaphore(checkBufferNumber(bufferNumber))) {

	m_buffers = new T*[m_bufferNumber];
	for (gsl::index i = 0; i < m_bufferNumber; i++) {
		m_buffers[i] = new T[m_bufferSize]{};
	}
}

// make sure, UINT_MAX + 1 is evenly divisible by m_bufferNumber
template<class T>
inline int CircularBuffer<T>::checkBufferNumber(int bufferNumber) {
	return pow(2,round(log2(bufferNumber)));
}

template<class T>
inline CircularBuffer<T>::~CircularBuffer() {
	for (gsl::index i = 0; i < m_bufferNumber; i++) {
		delete[] m_buffers[i];
	}
	delete[] m_buffers;
	delete m_freeBuffers;
	delete m_usedBuffers;
}

template<class T>
inline T * CircularBuffer<T>::getWriteBuffer() {
	// return pointer to buffer and increment m_writeCount
	return m_buffers[m_writeCount++ % m_bufferNumber];
}

template<class T>
inline T * CircularBuffer<T>::getReadBuffer() {
	// return pointer to buffer and increment m_readCount
	return m_buffers[m_readCount++ % m_bufferNumber];
}
#endif //CIRCULARBUFFER_H