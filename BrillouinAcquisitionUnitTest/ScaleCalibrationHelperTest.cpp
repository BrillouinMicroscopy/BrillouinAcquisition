#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\Acquisition\AcquisitionModes\ScaleCalibrationHelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{
	TEST_CLASS(ScaleCalibrationHelperUnitTest) {
	public:

		TEST_METHOD(initializeFromMicrometer_Simple) {
			auto scaleCalibration = ScaleCalibrationData{};

			scaleCalibration.micrometerToPixX = { 5, 0 };
			scaleCalibration.micrometerToPixY = { 0, 5 };

			ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

			// Does not change micrometerToPix values
			Assert::AreEqual(5.0, scaleCalibration.micrometerToPixX.x);
			Assert::AreEqual(0.0, scaleCalibration.micrometerToPixX.y);

			Assert::AreEqual(0.0, scaleCalibration.micrometerToPixY.x);
			Assert::AreEqual(5.0, scaleCalibration.micrometerToPixY.y);

			// Calculates correct pixToMicrometer values
			Assert::AreEqual(0.2, scaleCalibration.pixToMicrometerX.x);
			Assert::AreEqual(0.0, scaleCalibration.pixToMicrometerX.y);

			Assert::AreEqual(0.0, scaleCalibration.pixToMicrometerY.x);
			Assert::AreEqual(0.2, scaleCalibration.pixToMicrometerY.y);

			// Check conversion
			auto posPixel = microMeterToPix(&scaleCalibration, { 1, 1 });

			Assert::AreEqual(5.0, posPixel.x, 1e-10);
			Assert::AreEqual(5.0, posPixel.y, 1e-10);

			auto posMicrometer = pixToMicroMeter(&scaleCalibration, { 10, 10 });

			Assert::AreEqual(2.0, posMicrometer.x, 1e-10);
			Assert::AreEqual(2.0, posMicrometer.y, 1e-10);

			// Check integrity
			checkIntegrity(&scaleCalibration);
		}

		TEST_METHOD(initializeFromMicrometer_Simple_Inverted) {
			auto scaleCalibration = ScaleCalibrationData{};

			scaleCalibration.micrometerToPixX = { 0, 5 };
			scaleCalibration.micrometerToPixY = { 5, 0 };

			ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

			// Does not change micrometerToPix values
			Assert::AreEqual(0.0, scaleCalibration.micrometerToPixX.x);
			Assert::AreEqual(5.0, scaleCalibration.micrometerToPixX.y);

			Assert::AreEqual(5.0, scaleCalibration.micrometerToPixY.x);
			Assert::AreEqual(0.0, scaleCalibration.micrometerToPixY.y);

			// Calculates correct pixToMicrometer values
			Assert::AreEqual(0.0, scaleCalibration.pixToMicrometerX.x);
			Assert::AreEqual(0.2, scaleCalibration.pixToMicrometerX.y);

			Assert::AreEqual(0.2, scaleCalibration.pixToMicrometerY.x);
			Assert::AreEqual(0.0, scaleCalibration.pixToMicrometerY.y);

			// Check conversion
			auto posPixel = microMeterToPix(&scaleCalibration, { 1, 1 });

			Assert::AreEqual(5.0, posPixel.x, 1e-10);
			Assert::AreEqual(5.0, posPixel.y, 1e-10);

			auto posMicrometer = pixToMicroMeter(&scaleCalibration, { 10, 10 });

			Assert::AreEqual(2.0, posMicrometer.x, 1e-10);
			Assert::AreEqual(2.0, posMicrometer.y, 1e-10);

			// Check integrity
			checkIntegrity(&scaleCalibration);
		}

		TEST_METHOD(initializeFromMicrometer_Rotated) {
			auto scaleCalibration = ScaleCalibrationData{};

			scaleCalibration.micrometerToPixX = { -10, 10 };
			scaleCalibration.micrometerToPixY = { 10, 10 };

			ScaleCalibrationHelper::initializeCalibrationFromMicrometer(&scaleCalibration);

			// Does not change micrometerToPix values
			Assert::AreEqual(-10.0, scaleCalibration.micrometerToPixX.x);
			Assert::AreEqual(10.0, scaleCalibration.micrometerToPixX.y);

			Assert::AreEqual(10.0, scaleCalibration.micrometerToPixY.x);
			Assert::AreEqual(10.0, scaleCalibration.micrometerToPixY.y);

			// Calculates correct pixToMicrometer values
			Assert::AreEqual(-0.05, scaleCalibration.pixToMicrometerX.x);
			Assert::AreEqual(0.05, scaleCalibration.pixToMicrometerX.y);

			Assert::AreEqual(0.05, scaleCalibration.pixToMicrometerY.x);
			Assert::AreEqual(0.05, scaleCalibration.pixToMicrometerY.y);

			// Check conversion
			auto posPixel = microMeterToPix(&scaleCalibration, { 1, 1 });

			Assert::AreEqual(0.0, posPixel.x, 1e-10);
			Assert::AreEqual(20.0, posPixel.y, 1e-10);

			auto posMicrometer = pixToMicroMeter(&scaleCalibration, { 10, 10 });

			Assert::AreEqual(0.0, posMicrometer.x, 1e-10);
			Assert::AreEqual(1.0, posMicrometer.y, 1e-10);

			checkIntegrity(&scaleCalibration);
		}

		TEST_METHOD(initializeFromPixel_Rotated) {
			auto scaleCalibration = ScaleCalibrationData{};

			scaleCalibration.pixToMicrometerX = { -10, 10 };
			scaleCalibration.pixToMicrometerY = { 10, 10 };

			ScaleCalibrationHelper::initializeCalibrationFromPixel(&scaleCalibration);

			// Does not change micrometerToPix values
			Assert::AreEqual(-10.0, scaleCalibration.pixToMicrometerX.x);
			Assert::AreEqual(10.0, scaleCalibration.pixToMicrometerX.y);

			Assert::AreEqual(10.0, scaleCalibration.pixToMicrometerY.x);
			Assert::AreEqual(10.0, scaleCalibration.pixToMicrometerY.y);

			// Calculates correct micrometerToPix values
			Assert::AreEqual(-0.05, scaleCalibration.micrometerToPixX.x);
			Assert::AreEqual(0.05, scaleCalibration.micrometerToPixX.y);

			Assert::AreEqual(0.05, scaleCalibration.micrometerToPixY.x);
			Assert::AreEqual(0.05, scaleCalibration.micrometerToPixY.y);

			// Check conversion
			auto posMicrometer = pixToMicroMeter(&scaleCalibration, { 1, 1 });

			Assert::AreEqual(0.0, posMicrometer.x, 1e-10);
			Assert::AreEqual(20.0, posMicrometer.y, 1e-10);

			auto posPixel = microMeterToPix(&scaleCalibration, { 10, 10 });

			Assert::AreEqual(0.0, posPixel.x, 1e-10);
			Assert::AreEqual(1.0, posPixel.y, 1e-10);

			checkIntegrity(&scaleCalibration);
		}

	private:
		void checkIntegrity(ScaleCalibrationData* scaleCalibration) {

			auto posMicrometer = POINT2{ 3.14, 42 };

			auto posPixel = posMicrometer.x * scaleCalibration->micrometerToPixX + posMicrometer.y * scaleCalibration->micrometerToPixY;
			auto posMicrometer2 = posPixel.x * scaleCalibration->pixToMicrometerX + posPixel.y * scaleCalibration->pixToMicrometerY;

			Assert::AreEqual(posMicrometer.x, posMicrometer2.x, 1e-10);
			Assert::AreEqual(posMicrometer.y, posMicrometer2.y, 1e-10);

			auto posPixel2 = POINT2{ 42, 3.14 };

			auto posMicrometer3 = posPixel2.x * scaleCalibration->pixToMicrometerX + posPixel2.y * scaleCalibration->pixToMicrometerY;
			auto posPixel3 = posMicrometer3.x * scaleCalibration->micrometerToPixX + posMicrometer3.y * scaleCalibration->micrometerToPixY;

			Assert::AreEqual(posPixel2.x, posPixel3.x, 1e-10);
			Assert::AreEqual(posPixel2.y, posPixel3.y, 1e-10);
		}

		POINT2 pixToMicroMeter(ScaleCalibrationData* scaleCalibration, POINT2 pix) {
			pix -= scaleCalibration->originPix;
			return pix.x * scaleCalibration->pixToMicrometerX + pix.y * scaleCalibration->pixToMicrometerY;
		}

		POINT2 microMeterToPix(ScaleCalibrationData* scaleCalibration, POINT2 microMeter) {
			return (microMeter.x * scaleCalibration->micrometerToPixX + microMeter.y * scaleCalibration->micrometerToPixY) + scaleCalibration->originPix;
		}
	};
}