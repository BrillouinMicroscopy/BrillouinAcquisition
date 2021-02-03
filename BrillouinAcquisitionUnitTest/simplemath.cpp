#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\simplemath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestSimpleMath) {
		public:
			// Tests for mean()
			TEST_METHOD(TestSolvingQuartic) {
				auto coef = COEFFICIENTS5{5, 4, 3, 2, -14};
				auto solutions = simplemath::solveQuartic(coef);
				auto expected = std::vector{
					std::complex<double>(-1.465836978554923, 0.0),
					std::complex<double>(1.0, 0.0),
					std::complex<double>(-0.167081510722539, -1.371953080493220),
					std::complex<double>(-0.167081510722539, 1.371953080493220)
				};
				for (int jj = 0; jj < expected.size(); jj++) {
					Assert::IsTrue(abs(expected[jj].real() - solutions[jj].real()) < 1e-12);
					Assert::IsTrue(abs(expected[jj].imag() - solutions[jj].imag()) < 1e-12);
				}
			}

			TEST_METHOD(TestMedianOdd) {
				auto list = std::vector<double>{ 1, 2, 3, 4, 5 };
				auto med = simplemath::median(std::begin(list), std::end(list));
				Assert::AreEqual(3.0, med);
			}

			TEST_METHOD(TestMedianEven) {
				auto list = std::vector<double>{ 1, 2, 3, 4, 5, 6 };
				auto med = simplemath::median(std::begin(list), std::end(list));
				Assert::AreEqual(3.5, med);
			}

			TEST_METHOD(TestSumDouble) {
				auto list = std::vector<double>{ 1, 2, 3, 4, 5, 6 };
				auto sum = simplemath::sum(list);
				Assert::AreEqual(21.0, sum);
			}

			TEST_METHOD(TestSumInt) {
				auto list = std::vector<int>{ -1, 2, 3, 4, 5, -6 };
				auto sum = simplemath::sum(list);
				Assert::AreEqual(7, sum);
			}

			TEST_METHOD(TestAbsSumDouble) {
				auto list = std::vector<double>{ 1, 2, 3, 4, 5, 6 };
				auto sum = simplemath::absSum(list);
				Assert::AreEqual(21.0, sum);
			}

			TEST_METHOD(TestAbsSumInt) {
				auto list = std::vector<int>{ -1, 2, 3, 4, 5, -6 };
				auto sum = simplemath::absSum(list);
				Assert::AreEqual(21, sum);
			}
	};
}