#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/src/wrapper/unwrap2.h"
#include "../BrillouinAcquisition/src/lib/math/simplemath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestUnwrap) {
		public:

			// Tests for unwrap2D()
			TEST_METHOD(TestUnwrap2D) {
				unwrap2Wrapper *m_unwrapper = new unwrap2Wrapper();
				int dim_x{ 2000 };
				int dim_y{ 2000 };

				// Create expected phase values
				std::vector<float> expected(dim_x * dim_y, 0);
				for (int x{ 0 }; x < dim_x; x++) {
					for (int y{ 0 }; y < dim_y; y++) {
						expected[x + y * dim_x] = x / 30.0;
					}
				}

				// Create phase array wrapped by 2 pi
				std::vector<float> wrapped = expected;
				for (int x{ 0 }; x < dim_x; x++) {
					for (int y{ 0 }; y < dim_y; y++) {
						wrapped[x + y * dim_x] = std::fmod(wrapped[x + y * dim_x], 2.0 * PI);
					}
				}

				// Create ouput array
				std::vector<float> output;
				output.resize(dim_x * dim_y);

				std::vector<unsigned char> mask(dim_x * dim_y, 0);
				m_unwrapper->unwrap2DWrapped(&wrapped[0], &output[0], dim_x, dim_y, false, false);

				// Shift values so the that lowest value is zero
				auto min = simplemath::minimum(output);
				for (int jj{ 0 }; jj < dim_x*dim_y; jj++) {
					output[jj] -= min;
				}

				auto diff = output;
				for (int jj{ 0 }; jj < dim_x*dim_y; jj++) {
					diff[jj] -= expected[jj];
				}
				auto sum = simplemath::absSum(diff);
				
				Assert::IsTrue(sum < 1e-5*dim_x*dim_y);
			}
	};
}