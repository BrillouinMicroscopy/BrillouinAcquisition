#include "stdafx.h"
#include "converter.h"

converter::converter() {
}

converter::~converter() {
	delete m_phase;
}

void converter::init() {
	m_phase = new phase();
}

void converter::convert(PreviewBuffer<std::byte>* previewBuffer, PLOT_SETTINGS* plotSettings) {
	{
		std::lock_guard<std::mutex> lockGuard(previewBuffer->m_mutex);
		// if no image is ready return immediately
		if (!previewBuffer->m_buffer->m_usedBuffers->tryAcquire()) {
			return;
		}

		if (previewBuffer->m_bufferSettings.bufferType == "unsigned short") {
			auto unpackedBuffer = reinterpret_cast<unsigned short*>(previewBuffer->m_buffer->getReadBuffer());
			conv(previewBuffer, plotSettings, unpackedBuffer);
		} else if (previewBuffer->m_bufferSettings.bufferType == "unsigned char") {
			auto unpackedBuffer = reinterpret_cast<unsigned char*>(previewBuffer->m_buffer->getReadBuffer());
			conv(previewBuffer, plotSettings, unpackedBuffer);
		} else if (previewBuffer->m_bufferSettings.bufferType == "unsigned int") {
			auto unpackedBuffer = reinterpret_cast<unsigned int*>(previewBuffer->m_buffer->getReadBuffer());
			conv(previewBuffer, plotSettings, unpackedBuffer);
		}

	}
}

void converter::updateBackground() {
	m_phase->m_updateBackground = true;
}

template <typename T>
void converter::conv(PreviewBuffer<std::byte>* previewBuffer, PLOT_SETTINGS* plotSettings, T* unpackedBuffer) {
	auto dim_x = previewBuffer->m_bufferSettings.roi.width_binned;
	auto dim_y = previewBuffer->m_bufferSettings.roi.height_binned;

	std::vector<float> converted(unpackedBuffer, unpackedBuffer + (size_t)dim_x*dim_y);
	switch (plotSettings->mode) {
	case DISPLAY_MODE::PHASE:
		m_phase->calculatePhase(unpackedBuffer, &converted, dim_x, dim_y);
		break;
	case DISPLAY_MODE::SPECTRUM:
		m_phase->calculateSpectrum(unpackedBuffer, &converted, dim_x, dim_y);
		break;
	default:
		break;
	}
	previewBuffer->m_buffer->m_freeBuffers->release();
	emit(s_converted(plotSettings, dim_x, dim_y, converted));
}