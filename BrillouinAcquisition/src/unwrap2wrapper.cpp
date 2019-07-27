#include "stdafx.h"
#include "unwrap2Wrapper.h"

unwrap2Wrapper::unwrap2Wrapper() {
}

unwrap2Wrapper::~unwrap2Wrapper() {
	if (m_pixel) {
		free(m_pixel);
	}
	if (m_edge) {
		free(m_edge);
	}
}

void unwrap2Wrapper::unwrap2DWrapped(double * wrapped_image, double * UnwrappedImage, unsigned char * input_mask, int image_width, int image_height, int wrap_around_x, int wrap_around_y) {
	int image_size = image_height * image_width;
	int No_of_Edges_initially = 2 * image_size;
	if (m_image_size != image_size) {
		if (m_initialized) {
			free(m_pixel);
		}
		m_image_size = image_size;
		m_pixel = (PIXELM *)calloc(m_image_size, sizeof(PIXELM));
	}
	if (m_No_of_Edges_initially != No_of_Edges_initially) {
		if (m_initialized) {
			free(m_edge);
		}
		m_No_of_Edges_initially = No_of_Edges_initially;
		m_edge = (EDGE *)calloc(m_No_of_Edges_initially, sizeof(EDGE));
	}
	m_initialized = true;
	unwrap2D(wrapped_image, UnwrappedImage, input_mask, image_width, image_height, wrap_around_x, wrap_around_y, m_edge, m_pixel);
}
