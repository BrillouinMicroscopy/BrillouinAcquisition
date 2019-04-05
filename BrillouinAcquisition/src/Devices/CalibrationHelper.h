#ifndef CALIBRATIONHELPER_H
#define CALIBRATIONHELPER_H

#include <gsl/gsl>
#include <string>

#include "../interpolation.h"

struct BOUNDS {
	double xMin{ -1e3 };	// [µm] minimal x-value
	double xMax{ 1e3 };		// [µm] maximal x-value
	double yMin{ -1e3 };	// [µm] minimal y-value
	double yMax{ 1e3 };		// [µm] maximal y-value
	double zMin{ -1e3 };	// [µm] minimal z-value
	double zMax{ 1e3 };		// [µm] maximal z-value
};

struct POSITION_MAPS {
	std::vector<double> x{ 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5 };
	std::vector<double> y{ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 };
};

struct VOLTAGE_MAPS {
	std::vector<double> Ux{ -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
	std::vector<double> Uy{ -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2 };
};

struct MicroscopeProperties {
	int width{ 1280 };			// [pix] image width
	int height{ 1024 };			// [pix] image height
	double pixelSize{ 4.8e-6 };	// [µm]  pixel size
	double mag = 58;			// [1]   magnification
};

struct SpatialCalibration {
	std::string date{ "" };
	POSITION_MAPS positions;
	WEIGHTS2<double> positions_weights;
	VOLTAGE_MAPS voltages;
	WEIGHTS2<double> voltages_weights;
	BOUNDS bounds = {
		-53,	// [µm] minimal x-value
		 53,	// [µm] maximal x-value
		-43,	// [µm] minimal y-value
		 43,	// [µm] maximal y-value
		 -1000,	// [µm] minimal z-value
		  1000	// [µm] maximal z-value
	};
	MicroscopeProperties microscopeProperties;
	bool valid = false;
};

class CalibrationHelper {

public:
	static void CalibrationHelper::calculateCalibrationBounds(SpatialCalibration *calibration) {
		double fac = 1e6 * calibration->microscopeProperties.pixelSize / calibration->microscopeProperties.mag / 2;
		calibration->bounds.xMin = -1 * fac * calibration->microscopeProperties.width;
		calibration->bounds.xMax = fac * calibration->microscopeProperties.width;
		calibration->bounds.yMin = -1 * fac * calibration->microscopeProperties.height;
		calibration->bounds.yMax = fac * calibration->microscopeProperties.height;
	}

	static void CalibrationHelper::calculateCalibrationWeights(SpatialCalibration *calibration) {
		Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> x = Eigen::Map<Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic>>(&calibration->positions.x[0], calibration->positions.x.size(), 1);
		Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> y = Eigen::Map<Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic>>(&calibration->positions.y[0], calibration->positions.y.size(), 1);
		Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> Ux = Eigen::Map<Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic>>(&calibration->voltages.Ux[0], calibration->voltages.Ux.size(), 1);
		Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> Uy = Eigen::Map<Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic>>(&calibration->voltages.Uy[0], calibration->voltages.Uy.size(), 1);

		/*
		 * Calculate the position weights
		 */
		calibration->positions_weights.x = interpolation::biharmonic_spline_calculate_weights(x, y, Ux);
		calibration->positions_weights.y = interpolation::biharmonic_spline_calculate_weights(x, y, Uy);

		/*
		 * Calculate the voltage weights
		 */
		calibration->voltages_weights.x = interpolation::biharmonic_spline_calculate_weights(Ux, Uy, x);
		calibration->voltages_weights.y = interpolation::biharmonic_spline_calculate_weights(Ux, Uy, y);
	}


};

#endif //CALIBRATIONHELPER_H