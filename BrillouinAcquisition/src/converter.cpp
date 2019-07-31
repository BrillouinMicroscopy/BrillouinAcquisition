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

void converter::convert(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, unsigned char* unpackedBuffer) {
	conv(previewBuffer, plotSettings, unpackedBuffer);
}

void converter::convert(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, unsigned short* unpackedBuffer) {
	conv(previewBuffer, plotSettings, unpackedBuffer);
}

template <typename T>
void converter::conv(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, T* unpackedBuffer) {
	int dim_x = previewBuffer->m_bufferSettings.roi.width;
	int dim_y = previewBuffer->m_bufferSettings.roi.height;

	std::vector<float> converted(unpackedBuffer, unpackedBuffer + dim_x*dim_y);
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
	emit(s_converted(previewBuffer, plotSettings, converted));
}