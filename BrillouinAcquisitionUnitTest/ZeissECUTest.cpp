#include "stdafx.h"
#include "CppUnitTest.h"
#include "MockMicroscope.h"
#include "..\BrillouinAcquisition\src\Devices\ScanControls\ZeissECU.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BrillouinAcquisitionUnitTest
{
	TEST_CLASS(ZeissECUUnitTest) {
	public:

		//TEST_METHOD(ScanControl_Connecting) {
		//	MockMicroscope *microscope = new MockMicroscope();
		//	ZeissECU *scanControl = new ZeissECU();
		//	scanControl->setDevice(microscope);
		//	bool isConnected = scanControl->connectDevice();
		//	Assert::AreEqual((bool)1, isConnected);
		//}

		//TEST_METHOD(ScanControl_TestPositionZero) {
		//	MockMicroscope *microscope = new MockMicroscope();
		//	ZeissECU *scanControl = new ZeissECU();
		//	scanControl->setDevice(microscope);
		//	scanControl->setPosition({ 0,0,0 });
		//	std::string buffer = microscope->readOutputBuffer();

		//	Assert::AreEqual(std::string("NPXT000000\rNPYT000000\rFPZD000000\r"), buffer);
		//}

		//TEST_METHOD(ScanControl_TestPositionPositive) {
		//	MockMicroscope *microscope = new MockMicroscope();
		//	ZeissECU *scanControl = new ZeissECU();
		//	scanControl->setDevice(microscope);
		//	scanControl->setPosition({ 100,100,100 });
		//	std::string buffer = microscope->readOutputBuffer();

		//	Assert::AreEqual(std::string("NPXT000190\rNPYT000190\rFPZD000FA0\r"), buffer);
		//}

		//TEST_METHOD(ScanControl_TestPositionNegative) {
		//	MockMicroscope *microscope = new MockMicroscope();
		//	ZeissECU *scanControl = new ZeissECU();
		//	scanControl->setDevice(microscope);
		//	scanControl->setPosition({ -100,-100,-100 });
		//	std::string buffer = microscope->readOutputBuffer();

		//	Assert::AreEqual(std::string("NPXTFFFE6F\rNPYTFFFE6F\rFPZDFFF05F\r"), buffer);
		//}
	};
}