#ifndef VOLTAGECALIBRATIONHELPER_H
#define VOLTAGECALIBRATIONHELPER_H

#include <gsl/gsl>
#include <string>

#include "../../lib/math/interpolation.h"

struct POSITION_MAPS {
	std::vector<double> x{ 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5 };
	std::vector<double> y{ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 };
};

struct VOLTAGE_MAPS {
	std::vector<double> Ux{ -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
	std::vector<double> Uy{ -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2 };
};

struct VoltageCalibrationData {
	std::string date{ "" };
	POSITION_MAPS positions;
	WEIGHTS2<double> positions_weights;
	VOLTAGE_MAPS voltages;
	WEIGHTS2<double> voltages_weights;
	bool valid{ false };
};

class VoltageCalibrationHelper {

public:
	static void calculateCalibrationWeights(VoltageCalibrationData* calibration) {
		if (calibration->positions.x.size() == 0) {
			return;
		}
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

#endif //VOLTAGECALIBRATIONHELPER_H