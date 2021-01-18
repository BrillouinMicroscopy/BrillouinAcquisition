#ifndef SCALECALIBRATIONHELPER_H
#define SCALECALIBRATIONHELPER_H

#include "../../POINTS.h"

struct ScaleCalibrationData {
	POINT2 micrometerToPixX{ 0, 0 };	// [pix/micrometer]
	POINT2 micrometerToPixY{ 0, 0 };	// [pix/micrometer]

	POINT2 pixToMicrometerX{ 0, 0 };	// [micrometer/pix]
	POINT2 pixToMicrometerY{ 0, 0 };	// [micrometer/pix]

	POINT2 originPix{ 0, 0 };			// [pix] origin on the camera image
};

struct Matrix2{
	double a{ 0 };
	double b{ 0 };
	double c{ 0 };
	double d{ 0 };
};

class ScaleCalibrationHelper {

public:
	static void ScaleCalibrationHelper::initializeCalibrationFromMicrometer(ScaleCalibrationData* calibration) {
		// Check that the given vectors are actually a basis
		if (!isBasis(calibration->micrometerToPixX, calibration->micrometerToPixY)) {
			throw std::exception("Provided vectors are not a basis.");
		}

		auto micrometerToPix = Matrix2{ calibration->micrometerToPixX.x, calibration->micrometerToPixY.x,
										calibration->micrometerToPixX.y, calibration->micrometerToPixY.y };
		auto inverted = invert(micrometerToPix);

		calibration->pixToMicrometerX = POINT2{ inverted.a, inverted.c };
		calibration->pixToMicrometerY = POINT2{ inverted.b, inverted.d };
	}

	static void ScaleCalibrationHelper::initializeCalibrationFromPixel(ScaleCalibrationData* calibration) {
		// Check that the given vectors are actually a basis
		if (!isBasis(calibration->pixToMicrometerX, calibration->pixToMicrometerY)) {
			throw std::exception("Provided vectors are not a basis.");
		}

		auto pixToMicrometer = Matrix2{ calibration->pixToMicrometerX.x, calibration->pixToMicrometerY.x,
										calibration->pixToMicrometerX.y, calibration->pixToMicrometerY.y };
		auto inverted = invert(pixToMicrometer);

		calibration->micrometerToPixX = POINT2{ inverted.a, inverted.c };
		calibration->micrometerToPixY = POINT2{ inverted.b, inverted.d };
	}

	static bool isBasis(POINT2 e_0, POINT2 e_1) {
		// Check that the absolute value of the determinant is not zero
		return abs(determinate({ e_0.x, e_1.x, e_0.y, e_1.y })) > 1e-10;
	}

	static double determinate(Matrix2 matrix) {
		return matrix.a * matrix.d - matrix.b * matrix.c;
	}

	static Matrix2 invert(Matrix2 matrix) {
		auto det = determinate(matrix);
		auto a = matrix.d / det;
		auto b = -1 * matrix.b / det;
		auto c = -1 * matrix.c / det;
		auto d = matrix.a / det;
		return Matrix2{a, b, c, d};
	}
};

#endif //SCALECALIBRATIONHELPER_H