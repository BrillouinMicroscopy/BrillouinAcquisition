#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\Devices\ZeissECU.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{		
	TEST_CLASS(TestZeissECUHelpers) {
	public:
		
		TEST_METHOD(TestMethod1) {
			std::string answer = helper::parse("PN000000\r", "N");
			std::string expected = "000000";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestMethod2) {
			std::string answer = helper::parse("PH001\r", "H");
			std::string expected = "001";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestMethod3) {
			std::string answer = helper::parse("PN000000", "N");
			std::string expected = "";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestParsingVersionString1) {
			std::string answer = helper::parse("PNMC V2.08\r", "N");
			std::string expected = "MC V2.08";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestParsingVersionString2) {
			std::string answer = helper::parse("PFZM_V2_04\r", "F");
			std::string expected = "ZM_V2_04";
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(TestHex2Dec) {
			int result = helper::hex2dec("0003E8");
			int expected = 1000;
			Assert::AreEqual(expected, result);
		}

		TEST_METHOD(TestDec2Hex) {
			std::string result = helper::dec2hex(1000,6);
			std::string expected = "0003E8";
			Assert::AreEqual(expected, result);
		}

		TEST_METHOD(TestDec2Hex_Negative) {
			// provoke an overflow --> wraps around
			std::string result = helper::dec2hex(-1000, 6);
			std::string expected = "FFFFFC18";
			Assert::AreEqual(expected, result);
		}

	};
}