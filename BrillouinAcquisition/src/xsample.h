#ifndef XSAMPLE_H
#define XSAMPLE_H

#include <gsl/gsl>

class xsample {

private:
	
public:
	xsample();
	~xsample();

	template <typename T_in = double, typename T_out = double>
	static void resample(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new);

};

#endif // XSAMPLE_H

template<typename T_in, typename T_out>
static inline void xsample::resample(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new) {
	// number of pixels to make up a resampled pixel
	double scaling_x = (double)dim_x / dim_x_new;
	double scaling_y = (double)dim_y / dim_y_new;
	double pixelArea = scaling_x * scaling_y;
	for (int x{ 0 }; x < dim_x_new; x++) {
		double xl = x * scaling_x;
		double xr = (x + 1) * scaling_x;
		for (int y{ 0 }; y < dim_y_new; y++) {
			double yt = y * scaling_y;
			double yb = (y + 1) * scaling_y;
			double pixValue{ 0 };
			// Average old pixels
			for (int xd{ (int)floor(xl) }; xd < (int)ceil(xr); xd++) {
				auto dx1 = (xd > xl) ? xd : xl;
				auto dx2 = (xd + 1 < xr) ? (xd + 1) : xr;
				double weight_x = dx2 - dx1;
				for (int yd{ (int)floor(yt) }; yd < (int)ceil(yb); yd++) {

					auto dy1 = (yd > yt) ? yd : yt;
					auto dy2 = (yd + 1 < yb) ? (yd + 1) : yb;
					double weight_y = dy2 - dy1;

					pixValue += in[xd + dim_x * yd] * weight_x * weight_y;
				}
			}
			out[x + dim_x_new * y] = pixValue / pixelArea;
		}
	}
}
