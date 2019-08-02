#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\BrillouinAcquisition\src\xsample.h"
#include "..\BrillouinAcquisition\src\simplemath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestDownSample) {
		public:

			TEST_METHOD(TestDownSample3x3_2x2) {
				xsample *m_xsample = new xsample();
				int dim_x{ 3 };
				int dim_y{ 3 };
				int dim_x_new{ 2 };
				int dim_y_new{ 2 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 0);
				for (int i{ 0 }; i < dim_x * dim_y; i++) {
					input[i] = i + 1;
				}

				std::vector<double> expected{ 5.25 / 2.25, 8.25 / 2.25, 14.25 / 2.25, 17.25 / 2.25 };

				std::vector<double> output(dim_x_new* dim_y_new, 0);
				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);
				
				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestDownSample4x4_2x2) {
				xsample *m_xsample = new xsample();
				int dim_x{ 4 };
				int dim_y{ 4 };
				int dim_x_new{ 2 };
				int dim_y_new{ 2 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 0);
				for (int i{ 0 }; i < dim_x * dim_y; i++) {
					input[i] = i + 1;
				}

				std::vector<double> expected{ 3.5, 5.5, 11.5, 13.5 };

				std::vector<double> output(dim_x_new* dim_y_new, 0);
				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				Assert::IsTrue(expected == output);
			}

			TEST_METHOD(TestDownSample640x512_320x256) {
				xsample *m_xsample = new xsample();
				int dim_x{ 640 };
				int dim_y{ 512 };
				int dim_x_new{ 320 };
				int dim_y_new{ 256 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 1);

				std::vector<double> expected(dim_x_new * dim_y_new, 1);

				std::vector<double> output(dim_x_new* dim_y_new, 0);

				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				// Sum difference vector
				auto diff = output;
				for (int jj{ 0 }; jj < diff.size(); jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff) / diff.size();

				Assert::IsTrue(sum < 1e-5);
			}

			TEST_METHOD(TestUpSample3x3_4x4) {
				xsample *m_xsample = new xsample();
				int dim_x{ 3 };
				int dim_y{ 3 };
				int dim_x_new{ 4 };
				int dim_y_new{ 4 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 0);
				for (int i{ 0 }; i < dim_x * dim_y; i++) {
					input[i] = i + 1;
				}

				// TODO
				std::vector<double> expected{	1, 1.25 / 0.75, 1.75 / 0.75, 3,
												3, 2.75 / 0.75, 3.25 / 0.75, 5,
												5, 4.25 / 0.75, 4.75 / 0.75, 7,
												7, 5.75 / 0.75, 6.25 / 0.75, 9 };

				std::vector<double> output(dim_x_new* dim_y_new, 0);
				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				// Sum difference vector
				auto diff = output;
				for (int jj{ 0 }; jj < diff.size(); jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff) / diff.size();

				Assert::IsTrue(sum < 1e-5);
			}

			TEST_METHOD(TestUpSample2x2_4x4) {
				xsample *m_xsample = new xsample();
				int dim_x{ 2 };
				int dim_y{ 2 };
				int dim_x_new{ 4 };
				int dim_y_new{ 4 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 0);
				for (int i{ 0 }; i < dim_x * dim_y; i++) {
					input[i] = i + 1;
				}

				std::vector<double> expected{ 1, 1, 2, 2, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 4, 4 };

				std::vector<double> output(dim_x_new* dim_y_new, 0);
				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				// Sum difference vector
				auto diff = output;
				for (int jj{ 0 }; jj < diff.size(); jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff) / diff.size();

				Assert::IsTrue(sum < 1e-5);
			}

			TEST_METHOD(TestUpSample2x2_5x5) {
				xsample *m_xsample = new xsample();
				int dim_x{ 2 };
				int dim_y{ 2 };
				int dim_x_new{ 5 };
				int dim_y_new{ 5 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 0);
				for (int i{ 0 }; i < dim_x * dim_y; i++) {
					input[i] = i + 1;
				}

				std::vector<double> expected{	1, 1, 1.5, 2, 2,
												1, 1, 1.5, 2, 2,
												2, 2, 2.5, 3, 3,
												3, 3, 3.5, 4, 4,
												3, 3, 3.5, 4, 4};

				std::vector<double> output(dim_x_new* dim_y_new, 0);
				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				// Sum difference vector
				auto diff = output;
				for (int jj{ 0 }; jj < diff.size(); jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff) / diff.size();

				Assert::IsTrue(sum < 1e-5);
			}

			TEST_METHOD(TestUpSample640x512_1280x1024) {
				xsample *m_xsample = new xsample();
				int dim_x{ 640 };
				int dim_y{ 512 };
				int dim_x_new{ 1280 };
				int dim_y_new{ 1024 };

				// Create expected phase values
				std::vector<double> input(dim_x * dim_y, 1);

				std::vector<double> expected(dim_x_new * dim_y_new, 1);

				std::vector<double> output(dim_x_new* dim_y_new, 0);

				m_xsample->resample(&input[0], &output[0], dim_x, dim_y, dim_x_new, dim_y_new);

				// Sum difference vector
				auto diff = output;
				for (int jj{ 0 }; jj < diff.size(); jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff) / diff.size();

				Assert::IsTrue(sum < 1e-5);
			}
	};
}