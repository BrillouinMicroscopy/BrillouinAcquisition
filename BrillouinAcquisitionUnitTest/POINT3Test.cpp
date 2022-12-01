#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/src/lib/math/points.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{
	TEST_CLASS(POINT3Test) {
	public:

		TEST_METHOD(POINT3_initialization) {
			auto pos = POINT3{ 1, 2, 3 };

			Assert::AreEqual(1.0, pos.x);
			Assert::AreEqual(2.0, pos.y);
			Assert::AreEqual(3.0, pos.z);
		}

		TEST_METHOD(POINT3_addition) {
			auto pos1 = POINT3{ 1, 2, 3 };
			auto pos2 = POINT3{ -1, 3, 4 };
			auto pos3 = pos1 + pos2;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);
			Assert::AreEqual(3.0, pos1.z);

			Assert::AreEqual(-1.0, pos2.x);
			Assert::AreEqual(3.0, pos2.y);
			Assert::AreEqual(4.0, pos2.z);

			Assert::AreEqual(0.0, pos3.x);
			Assert::AreEqual(5.0, pos3.y);
			Assert::AreEqual(7.0, pos3.z);

			pos3 += POINT3{ 1, 1, 1 };

			Assert::AreEqual(1.0, pos3.x);
			Assert::AreEqual(6.0, pos3.y);
			Assert::AreEqual(8.0, pos3.z);
		}

		TEST_METHOD(POINT3_subtraction) {
			auto pos1 = POINT3{ 1, 2, 3 };
			auto pos2 = POINT3{ -1, 3, 4 };
			auto pos3 = pos1 - pos2;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);
			Assert::AreEqual(3.0, pos1.z);

			Assert::AreEqual(-1.0, pos2.x);
			Assert::AreEqual(3.0, pos2.y);
			Assert::AreEqual(4.0, pos2.z);

			Assert::AreEqual(2.0, pos3.x);
			Assert::AreEqual(-1.0, pos3.y);
			Assert::AreEqual(-1.0, pos3.z);

			pos3 -= POINT3{ 1, 1, 1 };

			Assert::AreEqual(1.0, pos3.x);
			Assert::AreEqual(-2.0, pos3.y);
			Assert::AreEqual(-2.0, pos3.z);
		}

		TEST_METHOD(POINT3_multiplication) {
			auto pos1 = POINT3{ 1, 2, 3 };
			auto pos2 = 2.0 * pos1;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);
			Assert::AreEqual(3.0, pos1.z);

			Assert::AreEqual(2.0, pos2.x);
			Assert::AreEqual(4.0, pos2.y);
			Assert::AreEqual(6.0, pos2.z);

			auto pos3 = pos1 * 3.0;

			Assert::AreEqual(3.0, pos3.x);
			Assert::AreEqual(6.0, pos3.y);
			Assert::AreEqual(9.0, pos3.z);
		}

		TEST_METHOD(POINT3_division) {
			auto pos1 = POINT3{ 2, 4, 6 };
			auto pos2 = pos1 / 2;

			Assert::AreEqual(2.0, pos1.x);
			Assert::AreEqual(4.0, pos1.y);
			Assert::AreEqual(6.0, pos1.z);

			Assert::AreEqual(1.0, pos2.x);
			Assert::AreEqual(2.0, pos2.y);
			Assert::AreEqual(3.0, pos2.z);

			pos2 /= 0.5;

			Assert::AreEqual(2.0, pos2.x);
			Assert::AreEqual(4.0, pos2.y);
			Assert::AreEqual(6.0, pos2.z);
		}

		TEST_METHOD(POINT3_absolute) {
			// Fun fact: This is a Pythagorean quadruple.
			auto pos = POINT3{ 1, 2, 2 };

			auto length = abs(pos);

			Assert::AreEqual(3.0, length, 1e-10);
		}
	};
}