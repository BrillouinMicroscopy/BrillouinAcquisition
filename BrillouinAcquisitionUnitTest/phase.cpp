#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\phase.h"
#include "..\BrillouinAcquisition\external\fftw\fftw3.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestCircShift) {
		public:
			// Tests for fftshift()
			TEST_METHOD(TestFftshiftOdd) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				auto output = input;
				phase::fftshift(&output[0], 3, 3);
				std::vector<double> expected{ 9, 7, 8, 3, 1, 2, 6, 4, 5 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestFftshiftEven) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				auto output = input;
				phase::fftshift(&output[0], 4, 4);
				std::vector<double> expected{ 11, 12, 9, 10, 15, 16, 13, 14, 3, 4, 1, 2, 7, 8, 5, 6 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestFftshiftEven_Complex) {
				int N = 16;
				fftw_complex* input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				for (int jj{ 0 }; jj < N; jj++) {
					input[jj][0] = jj + 1;
					input[jj][1] = jj + 1;
				}
				fftw_complex* expected = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				phase::fftshift(&input[0], 4, 4);
				std::vector<double> expected_0{ 11, 12, 9, 10, 15, 16, 13, 14, 3, 4, 1, 2, 7, 8, 5, 6 };
				std::vector<double> output_real = expected_0;
				std::vector<double> output_complex = expected_0;
				for (int jj{ 0 }; jj < N; jj++) {
					output_real[jj] = input[jj][0];
					output_complex[jj] = input[jj][1];
				}
				Assert::IsTrue(expected_0 == output_real);
				Assert::IsTrue(expected_0 == output_complex);
			}

			// Tests for ifftshift()
			TEST_METHOD(TestIfftshiftOdd) {
				std::vector<double> input{ 9, 7, 8, 3, 1, 2, 6, 4, 5 };
				auto output = input;
				phase::ifftshift(&output[0], 3, 3);
				std::vector<double> expected{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestIfftshiftEven) {
				std::vector<double> input{ 11, 12, 9, 10, 15, 16, 13, 14, 3, 4, 1, 2, 7, 8, 5, 6 };
				auto output = input;
				phase::ifftshift(&output[0], 4, 4);
				std::vector<double> expected{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				Assert::IsTrue(expected == output);
			}

			// Tests for circshift()
			TEST_METHOD(TestCircshiftOdd2) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				auto output = input;
				phase::circshift(&output[0], 3, 3, 2, 2);
				std::vector<double> expected{ 5, 6, 4, 8, 9, 7, 2, 3, 1 };
				Assert::IsTrue(expected == output);
			}

			// Tests for circshift()
			TEST_METHOD(TestCircshiftOdd3) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				auto output = input;
				phase::circshift(&output[0], 3, 3, 3, 3);
				std::vector<double> expected{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestCircshiftEven1) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				auto output = input;
				phase::circshift(&output[0], 4, 4, 1, 1);
				std::vector<double> expected{ 16, 13, 14, 15, 4, 1, 2, 3, 8, 5, 6, 7, 12, 9, 10, 11 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestCircshiftEven3) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				auto output = input;
				phase::circshift(&output[0], 4, 4, 3, 3);
				std::vector<double> expected{ 6, 7, 8, 5, 10, 11, 12, 9, 14, 15, 16, 13, 2, 3, 4, 1 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestCircshiftEven4) {
				std::vector<double> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				auto output = input;
				phase::circshift(&output[0], 4, 4, 4, 4);
				std::vector<double> expected{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestCircshiftOdd2_Complex) {
				int N = 9;
				fftw_complex* input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				for (int jj{ 0 }; jj < N; jj++) {
					input[jj][0] = jj + 1;
					input[jj][1] = jj + 1;
				}
				fftw_complex* expected = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				phase::circshift(&input[0], 3, 3, 2, 2);
				std::vector<double> expected_0{ 5, 6, 4, 8, 9, 7, 2, 3, 1 };
				std::vector<double> output_real = expected_0;
				std::vector<double> output_complex = expected_0;
				for (int jj{ 0 }; jj < N; jj++) {
					output_real[jj] = input[jj][0];
					output_complex[jj] = input[jj][1];
				}
				Assert::IsTrue(expected_0 == output_real);
				Assert::IsTrue(expected_0 == output_complex);
			}

			TEST_METHOD(TestCircshiftOdd3_Complex) {
				int N = 9;
				fftw_complex* input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				for (int jj{ 0 }; jj < N; jj++) {
					input[jj][0] = jj + 1;
					input[jj][1] = jj + 1;
				}
				fftw_complex* expected = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				phase::circshift(&input[0], 3, 3, 3, 3);
				std::vector<double> expected_0{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
				std::vector<double> output_real = expected_0;
				std::vector<double> output_complex = expected_0;
				for (int jj{ 0 }; jj < N; jj++) {
					output_real[jj] = input[jj][0];
					output_complex[jj] = input[jj][1];
				}
				Assert::IsTrue(expected_0 == output_real);
				Assert::IsTrue(expected_0 == output_complex);
			}

			TEST_METHOD(TestCircshiftEven1_Complex) {
				int N = 16;
				fftw_complex* input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				for (int jj{ 0 }; jj < N; jj++) {
					input[jj][0] = jj + 1;
					input[jj][1] = jj + 1;
				}
				fftw_complex* expected = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
				phase::circshift(&input[0], 4, 4, 1, 1);
				std::vector<double> expected_0{ 16, 13, 14, 15, 4, 1, 2, 3, 8, 5, 6, 7, 12, 9, 10, 11 };
				std::vector<double> output_real = expected_0;
				std::vector<double> output_complex = expected_0;
				for (int jj{ 0 }; jj < N; jj++) {
					output_real[jj] = input[jj][0];
					output_complex[jj] = input[jj][1];
				}
				Assert::IsTrue(expected_0 == output_real);
				Assert::IsTrue(expected_0 == output_complex);
			}

			//TEST_METHOD(TestCircshiftEven4_Complex) {
			//	fftw_complex* input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * 9);
			//	for (int jj{ 0 }; jj < 16; jj++) {
			//		input[jj][0] = jj;
			//		input[jj][1] = jj;
			//	}
			//	auto output = input;
			//	auto expected = input;
			//	phase::circshift(&output[0], 4, 4, 4, 4);
			//	std::vector<double> expected_0{ 16, 13, 14, 15, 4, 1, 2, 3, 8, 5, 6, 7, 12, 9, 10, 11 };
			//	for (int jj{ 0 }; jj < 9; jj++) {
			//		expected[jj][0] = jj;
			//		expected[jj][1] = jj;
			//	}
			//	Assert::IsTrue(expected == output);
			//}
	};
}