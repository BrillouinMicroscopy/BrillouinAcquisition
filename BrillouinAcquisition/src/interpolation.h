#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <gsl/gsl>

#include <complex>
#include <cmath>
#include "../external/eigen/Eigen/Dense"

class interpolation {
public:
	/*
	 * Biharmonic spline interpolation
	 *
	 * Reference:
	 * David T. Sandwell, "Biharmonic spline
	 * interpolation of GEOS-3 and SEASAT altimeter data",
	 * Geophysical Research Letters, 2, 139-142, 1987.
	 */
	template <typename T = double>
	static Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> biharmonic_spline(
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> x,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> y,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> values,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> xr,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> yr
	) {
		// Create vectors from position and value matrices
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> x_vec(x.data(), x.size());
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> y_vec(y.data(), y.size());
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> values_vec(values.data(), values.size());

		// Calculate distances between positions
		auto xy_vec = x_vec + std::complex<T>(0, 1) * y_vec;
		auto xy = xy_vec.replicate(x.size(), 1);
		auto d = (xy.array() - xy.transpose().array()).abs();

		// Calculate interpolation weights
		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> g = (d.square() * (d.log() - 1)).matrix();
		g.diagonal().setZero();
		Eigen::Matrix<T, Eigen::Dynamic, 1> weights = g.colPivHouseholderQr().solve(values_vec.transpose());

		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> valuesr(xr.rows(), xr.cols());
		valuesr.setConstant(NAN);

		// Calculate the values at the requested points
		xy.transpose();
		for (gsl::index rowNumber{ 0 }; rowNumber < valuesr.rows(); rowNumber++) {
			for (gsl::index colNumber{ 0 }; colNumber < valuesr.cols(); colNumber++) {
				Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> d = (xr(rowNumber, colNumber) + std::complex<T>(0, 1) * yr(rowNumber, colNumber) - xy_vec.array()).abs();
				Eigen::Matrix<T, 1, Eigen::Dynamic> g = (d.square() * (d.log() - 1)).matrix();
				
				// Set the value of the Green's function at position zero
				Eigen::Index minRow, minCol;
				d.abs().minCoeff(&minRow, &minCol);
				if (d(minRow, minCol) == 0.0) {
					g(minRow, minCol) = 0;
				}

				Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> tmp = g * weights;
				valuesr(rowNumber, colNumber) = g * weights;
			}
		}

		return valuesr;
	}
};

#endif //INTERPOLATION_H