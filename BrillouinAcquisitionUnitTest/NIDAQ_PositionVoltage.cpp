#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/NIDAQ.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{		
	TEST_CLASS(TestNIDAQPositionCalculation) {
	public:
		
		TEST_METHOD(TestVtoP_center) {
			NIDAQ *nidaq = new NIDAQ();
			nidaq->loadCalibration("");
			VOLTAGE2 voltage{ 0, 0 };
			POINT2 answer = nidaq->voltageToPosition(voltage);
			POINT2 expected{ 0, 0 };
			Assert::AreEqual(expected.x, answer.x);
			Assert::AreEqual(expected.y, answer.y);
		}

	};
}