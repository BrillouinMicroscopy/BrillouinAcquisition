#include "stdafx.h"
#include "CppUnitTest.h"
#include "MockMicroscope.h"
#include "../BrillouinAcquisition/scancontrol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{
	TEST_CLASS(ScanControlUnitTest) {
	public:

		TEST_METHOD(ScanControl_Connecting) {
			MockMicroscope *microscope = new MockMicroscope();
			ScanControl *scanControl = new ScanControl();
			scanControl->setDevice(microscope);
			bool isConnected = scanControl->connect();
			Assert::AreEqual((bool)1, isConnected);
		}

		TEST_METHOD(ScanControl_TestPosition) {
			MockMicroscope *microscope = new MockMicroscope();
			ScanControl *scanControl = new ScanControl();
			scanControl->setDevice(microscope);
			std::vector<double> position = scanControl->getPosition();
			std::vector<double> expected = { 0, 0, 0 };
			Assert::IsTrue(expected == position);
		}
	};
}