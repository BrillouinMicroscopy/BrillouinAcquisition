#ifndef XSAMPLE_H
#define XSAMPLE_H

#include <gsl/gsl>

class xsample {

private:
	
public:
	xsample();
	~xsample();

	template <typename T_in = double, typename T_out = double>
	void down(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new);

	template <typename T_in = double, typename T_out = double>
	void up(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new);
};

#endif // XSAMPLE_H

// Naive implementations, they will fail hard when dim != 2*dim_new
template<typename T_in, typename T_out>
inline void xsample::down(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new) {
	for (int x{ 0 }; x < dim_x_new; x++) {
		for (int y{ 0 }; y < dim_y_new; y++) {
			out[x + dim_x_new * y] = in[2 * x + dim_x * y * 2];
		}
	}
}

template<typename T_in, typename T_out>
inline void xsample::up(T_in in, T_out out, int dim_x, int dim_y, int dim_x_new, int dim_y_new) {
	for (int x{ 0 }; x < dim_x_new; x++) {
		for (int y{ 0 }; y < dim_y_new; y++) {
			out[x + dim_x_new * y] = in[(x / 2) + dim_x * (y / 2)];
		}
	}
}
