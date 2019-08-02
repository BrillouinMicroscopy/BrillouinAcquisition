#ifndef UNWRAPPER_H
#define UNWRAPPER_H

#include <gsl/gsl>

extern "C" {
	#include "../external/unwrap/unwrap2D.h"
}

class unwrap2Wrapper {

private:
	bool m_initialized{ false };
	int m_image_size{ 0 };
	int m_No_of_Edges_initially{ 0 };
	PIXELM *m_pixel = nullptr;
	EDGE *m_edge = nullptr;
	
public:
	unwrap2Wrapper();
	~unwrap2Wrapper();

	void unwrap2DWrapped(float *wrapped_image, float *UnwrappedImage,
		int image_width, int image_height,
		int wrap_around_x, int wrap_around_y);
};

#endif // UNWRAPPER_H