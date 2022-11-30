#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BrillouinAcquisition/src/lib/math/points.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{
	TEST_CLASS(POINT2Test) {
	public:

		TEST_METHOD(POINT2_initialization) {
			auto pos = POINT2{ 1, 2 };

			Assert::AreEqual(1.0, pos.x);
			Assert::AreEqual(2.0, pos.y);
		}

		TEST_METHOD(POINT2_addition) {
			auto pos1 = POINT2{ 1, 2 };
			auto pos2 = POINT2{ -1, 3 };
			auto pos3 = pos1 + pos2;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);

			Assert::AreEqual(-1.0, pos2.x);
			Assert::AreEqual(3.0, pos2.y);

			Assert::AreEqual(0.0, pos3.x);
			Assert::AreEqual(5.0, pos3.y);

			pos3 += POINT2{ 1, 1 };

			Assert::AreEqual(1.0, pos3.x);
			Assert::AreEqual(6.0, pos3.y);
		}

		TEST_METHOD(POINT2_subtraction) {
			auto pos1 = POINT2{ 1, 2 };
			auto pos2 = POINT2{ -1, 3 };
			auto pos3 = pos1 - pos2;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);

			Assert::AreEqual(-1.0, pos2.x);
			Assert::AreEqual(3.0, pos2.y);

			Assert::AreEqual(2.0, pos3.x);
			Assert::AreEqual(-1.0, pos3.y);

			pos3 -= POINT2{ 1, 1 };

			Assert::AreEqual(1.0, pos3.x);
			Assert::AreEqual(-2.0, pos3.y);
		}

		TEST_METHOD(POINT2_multiplication) {
			auto pos1 = POINT2{ 1, 2 };
			auto pos2 = 2.0 * pos1;

			Assert::AreEqual(1.0, pos1.x);
			Assert::AreEqual(2.0, pos1.y);

			Assert::AreEqual(2.0, pos2.x);
			Assert::AreEqual(4.0, pos2.y);

			auto pos3 = pos1 * 3.0;

			Assert::AreEqual(3.0, pos3.x);
			Assert::AreEqual(6.0, pos3.y);
		}

		TEST_METHOD(POINT2_division) {
			auto pos1 = POINT2{ 2, 4 };
			auto pos2 = pos1 / 2;

			Assert::AreEqual(2.0, pos1.x);
			Assert::AreEqual(4.0, pos1.y);

			Assert::AreEqual(1.0, pos2.x);
			Assert::AreEqual(2.0, pos2.y);

			pos2 /= 0.5;

			Assert::AreEqual(2.0, pos2.x);
			Assert::AreEqual(4.0, pos2.y);
		}

		TEST_METHOD(POINT2_absolute) {
			// Fun fact: This is a Pythagorean triple.
			auto pos = POINT2{ 3, 4 };

			auto length = abs(pos);

			Assert::AreEqual(5.0, length, 1e-10);
		}
	};
}