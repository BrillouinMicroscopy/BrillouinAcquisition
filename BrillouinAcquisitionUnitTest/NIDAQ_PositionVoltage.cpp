#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\Devices\NIDAQ.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{		
	TEST_CLASS(TestNIDAQPositionCalculation) {
	public:
		
		TEST_METHOD(TestVtoP_center) {
			NIDAQ *nidaq = new NIDAQ();
			nidaq->loadVoltagePositionCalibration("");
			VOLTAGE2 voltage{ 0, 0 };
			POINT2 answer = nidaq->voltageToPosition(voltage);
			POINT2 expected{ 0, 0 };
			Assert::IsTrue(abs(expected.x - answer.x) < 1e-5);
			Assert::IsTrue(abs(expected.y - answer.y) < 1e-5);
		}

		TEST_METHOD(TestVtoV_center) {
			NIDAQ *nidaq = new NIDAQ();
			nidaq->loadVoltagePositionCalibration("");
			VOLTAGE2 voltage{ 0, 0 };
			POINT2 position = nidaq->voltageToPosition(voltage);
			VOLTAGE2 voltage2 = nidaq->positionToVoltage(position);
			Assert::AreEqual(voltage.Ux, voltage2.Ux);
			Assert::AreEqual(voltage.Uy, voltage2.Uy);
		}

		TEST_METHOD(TestVtoV) {
			NIDAQ *nidaq = new NIDAQ();
			VOLTAGE2 voltage_in{ 0.01, 0.01 };
			POINT2 position = nidaq->voltageToPosition(voltage_in);
			VOLTAGE2 voltage_out = nidaq->positionToVoltage(position);
			Assert::IsTrue(abs(voltage_in.Ux - voltage_out.Ux) < 1e-12);
			Assert::IsTrue(abs(voltage_in.Uy - voltage_out.Uy) < 1e-12);
		}

	};
}