#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\simplemath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestSimpleMath) {
		public:
			// Tests for mean()
			TEST_METHOD(TestSolvingQuartic) {
				COEFFICIENTS5 coef = {5, 4, 3, 2, -14};
				std::vector<std::complex<double>> solutions = simplemath::solveQuartic(coef);
				std::vector<std::complex<double>> expected = {
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
				std::vector<double> list{ 1, 2, 3, 4, 5 };
				auto med = simplemath::median(std::begin(list), std::end(list));
				Assert::AreEqual(med, 3.0);
			}

			TEST_METHOD(TestMedianEven) {
				std::vector<double> list{ 1, 2, 3, 4, 5, 6 };
				auto med = simplemath::median(std::begin(list), std::end(list));
				Assert::AreEqual(med, 3.5);
			}
	};
}