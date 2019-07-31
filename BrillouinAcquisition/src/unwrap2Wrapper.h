#ifndef UNWRAPPER_H
#define UNWRAPPER_H

#include <gsl/gsl>

extern "C" {
	#include "../external/unwrap/unwrap2D.h"
	#include "../external/unwrap2/residues.h"
	#include "../external/unwrap2/pi.h"
	#include "../external/unwrap2/maskfat.h"
	#include "../external/unwrap2/extract.h"
	#include "../external/unwrap2/gold.h"
	#include "../external/unwrap2/dipole.h"
	#include "../external/unwrap2/path.h"
	#include "../external/unwrap2/list.h"
	#include "../external/unwrap2/util.h"
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

	void unwrap2DWrapped(double *wrapped_image, double *UnwrappedImage, unsigned char *input_mask,
		int image_width, int image_height,
		int wrap_around_x, int wrap_around_y);

	void unwrap2DWrappedFast(float *phase, float *soln, unsigned char *input_mask,
		int image_width, int image_height);
};

#endif // UNWRAPPER_H