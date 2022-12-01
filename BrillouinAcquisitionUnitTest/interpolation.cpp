#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/src/lib/math/interpolation.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest {

	TEST_CLASS(TestInterpolation) {
		public:
			// Tests for biharmonic_spline()
			TEST_METHOD(TestInterpolation_Trivial) {
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> x(3, 3);
				x << 1, 2, 3, 4, 5, 6, 7, 8, 9;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> y(3, 3);
				y << 1, 4, 7, 2, 5, 8, 3, 6, 9;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> values = x.square();
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> xr = x;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> yr = y;
				auto valuesr = interpolation::biharmonic_spline(x, y, values, xr, yr);
				auto res = (values - valuesr).abs().sum();
				Assert::IsTrue(res < 1e-10);
			}

			TEST_METHOD(TestInterpolation_NonTrivial) {
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> x(3, 3);
				x << 1, 2, 3, 4, 5, 6, 7, 8, 9;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> y(3, 3);
				y << 1, 4, 7, 2, 5, 8, 3, 6, 9;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> values = x.square();
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> xr(2, 2);
				xr << 1.5, 2.5, 3.5, 4.5;
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> yr = xr;
				auto valuesr = interpolation::biharmonic_spline(x, y, values, xr, yr);
				Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic> expectedValues(2, 2);
				expectedValues << 4.999647573359866, 8.747036390727300, 13.124371569773734, 20.481440451142680;
				auto res = (expectedValues - valuesr).abs().sum();
				Assert::IsTrue(res < 1e-10);
			}
	};
}