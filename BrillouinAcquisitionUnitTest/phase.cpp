#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\phase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestCircShift) {
		public:
			// Tests for circshift()
			TEST_METHOD(TestCircShiftRightOdd) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				auto output = input;
				phase::circshift(&output[0], 3, 3, { 1, 1 });
				std::vector<double> expected{ 9, 7, 8, 3, 1, 2, 6, 4, 5 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestCircShiftRightEven) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				auto output = input;
				phase::circshift(&output[0], 4, 4, { 1, 1 });
				std::vector<double> expected{ 11, 12, 9, 10, 15, 16, 13, 14, 3, 4, 1, 2, 7, 8, 5, 6 };
				Assert::IsTrue(expected == output);
			}
	};
}