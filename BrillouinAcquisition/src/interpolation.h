#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <gsl/gsl>

#include <complex>
#include <cmath>
#include "../external/eigen/Eigen/Dense"

template <typename T = double>
struct WEIGHTS {
	Eigen::Matrix<T, Eigen::Dynamic, 1> weights;
	Eigen::Matrix<std::complex<T>, Eigen::Dynamic, Eigen::Dynamic> xy_vec;
};

template <typename T = double>
struct WEIGHTS2 {
	WEIGHTS<T> x;
	WEIGHTS<T> y;
};

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
		WEIGHTS<T> weights = biharmonic_spline_calculate_weights(x, y, values);

		return biharmonic_spline_calculate_values(weights, xr, yr);
	}

	template <typename T = double>
	static WEIGHTS<T> biharmonic_spline_calculate_weights(
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> x,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> y,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> values
	) {
		// Create vectors from position and value matrices
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> x_vec(x.data(), x.size());
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> y_vec(y.data(), y.size());
		Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic>> values_vec(values.data(), values.size());

		WEIGHTS<T> weights;

		// Calculate distances between positions
		weights.xy_vec = x_vec + std::complex<T>(0, 1) * y_vec;
		auto xy = weights.xy_vec.replicate(x.size(), 1);
		auto d = (xy - xy.transpose()).array().abs();

		// Calculate interpolation weights
		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> g = (d.square() * (d.log() - 1)).matrix();
		g.diagonal().setZero();
		weights.weights = g.colPivHouseholderQr().solve(values_vec.transpose());
		
		return weights;
	}

	template <typename T = double>
	static Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> biharmonic_spline_calculate_values(
		WEIGHTS<T> weights,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> xr,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> yr
	) {
		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> values(xr.rows(), xr.cols());
		values.setConstant(NAN);

		// Calculate the values at the requested points
		for (gsl::index rowNumber{ 0 }; rowNumber < values.rows(); rowNumber++) {
			for (gsl::index colNumber{ 0 }; colNumber < values.cols(); colNumber++) {
				Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> d
					= (xr(rowNumber, colNumber) + std::complex<T>(0, 1) * yr(rowNumber, colNumber) - weights.xy_vec.array()).abs();
				Eigen::Matrix<T, 1, Eigen::Dynamic> g = (d.square() * (d.log() - 1)).matrix();

				// Set the value of the Green's function at position zero
				Eigen::Index minRow, minCol;
				d.abs().minCoeff(&minRow, &minCol);
				if (d(minRow, minCol) == 0.0) {
					g(minRow, minCol) = 0;
				}

				values(rowNumber, colNumber) = g * weights.weights;
			}
		}

		return values;
	}

	template <typename T = double>
	static T biharmonic_spline_calculate_values(
		WEIGHTS<T> weights,
		T xr,
		T yr
	) {
		if (weights.weights.size() == 0) {
			return 0.0;
		}
		// Calculate the value for the requested point
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic> d
			= (xr + std::complex<T>(0, 1) * yr - weights.xy_vec.array()).abs();
		Eigen::Matrix<T, 1, Eigen::Dynamic> g = (d.square() * (d.log() - 1)).matrix();

		// Set the value of the Green's function at position zero
		Eigen::Index minRow, minCol;
		d.abs().minCoeff(&minRow, &minCol);
		if (d(minRow, minCol) == 0.0) {
			g(minRow, minCol) = 0;
		}

		return g * weights.weights;
	}
};

#endif //INTERPOLATION_H