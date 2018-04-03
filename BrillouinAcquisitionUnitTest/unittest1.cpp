#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/scancontrol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{		
	TEST_CLASS(UnitTest1) {
	public:
		
		TEST_METHOD(TestMethod1) {
			com *comObject = new com();
			MCU *mcu = new MCU(comObject);
			std::string answer = mcu->parse("PND");
			std::string expected = "D";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestMethod2) {
			com *comObject = new com();
			Stand *stand = new Stand(comObject);
			std::string answer = stand->parse("PI001");
			std::string expected = "001";
			Assert::AreEqual(expected, answer);
		}

	};
}