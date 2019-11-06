#include "stdafx.h"
#include "ODTControl.h"

/*
 * Public definitions
 */

void ODTControl::setVoltage(VOLTAGE2 voltages) {
	DAQmxStopTask(AOtaskHandle);
	float64 data[2] = { voltages.Ux, voltages.Uy };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
}

/*
 * Public slots
 */

void ODTControl::connectDevice() {
	// Create task for analog output
	DAQmxCreateTask("AO", &AOtaskHandle);
	// Configure analog output channels
	DAQmxCreateAOVoltageChan(AOtaskHandle, "Dev1/ao0:1", "AO", -1.0, 1.0, DAQmx_Val_Volts, "");
	// Configure sample rate to 1000 Hz
	DAQmxCfgSampClkTiming(AOtaskHandle, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);

	// Set analog output to zero
	float64 data[2] = { 0, 0 };
	DAQmxWriteAnalogF64(AOtaskHandle, 1, false, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);

	DAQmxSetWriteAttribute(AOtaskHandle, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen);

	// Create task for digital output
	DAQmxCreateTask("DO", &DOtaskHandle);
	// Configure digital output channel
	DAQmxCreateDOChan(DOtaskHandle, "Dev1/Port0/Line0:0", "DO", DAQmx_Val_ChanForAllLines);
	// Configure sample rate to 1000 Hz
	DAQmxCfgSampClkTiming(DOtaskHandle, "/Dev1/ao/SampleClock", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000);

	// Set digital line to low
	DAQmxWriteDigitalLines(DOtaskHandle, 1, false, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);

	DAQmxSetWriteAttribute(DOtaskHandle, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen);

	// Create task for digital output to LED lamp
	DAQmxCreateTask("DO_LED", &DOtaskHandle_LED);
	// Configure digital output channel
	DAQmxCreateDOChan(DOtaskHandle_LED, "Dev1/Port0/Line2:2", "DO_LED", DAQmx_Val_ChanForAllLines);
	// Configure regen mode
	DAQmxSetWriteAttribute(DOtaskHandle_LED, DAQmx_Write_RegenMode, DAQmx_Val_AllowRegen);
	// Start digital task
	DAQmxStartTask(DOtaskHandle_LED);
	// Set digital line to low
	DAQmxWriteDigitalLines(DOtaskHandle_LED, 1, false, 10, DAQmx_Val_GroupByChannel, &m_TTL.low, NULL, NULL);

	// Start digital task
	DAQmxStartTask(DOtaskHandle);

	// Start analog task after digital task since AO is the master
	DAQmxStartTask(AOtaskHandle);
}

void ODTControl::disconnectDevice() {
	// Stop and clear DAQ tasks
	DAQmxStopTask(AOtaskHandle);
	DAQmxClearTask(AOtaskHandle);
	DAQmxStopTask(DOtaskHandle);
	DAQmxClearTask(DOtaskHandle);
}

void ODTControl::setLEDLamp(bool position) {
	m_LEDon = position;
	// Write digital voltages
	const uInt8	voltage = (uInt8)m_LEDon;
	DAQmxWriteDigitalLines(DOtaskHandle_LED, 1, false, 10, DAQmx_Val_GroupByChannel, &voltage, NULL, NULL);
}

int ODTControl::getLEDLamp() {
	return (int)m_LEDon;
}

void ODTControl::setAcquisitionVoltages(ACQ_VOLTAGES voltages) {
	// Stop DAQ tasks
	DAQmxStopTask(AOtaskHandle);
	DAQmxStopTask(DOtaskHandle);

	// Write analog voltages
	DAQmxWriteAnalogF64(AOtaskHandle, voltages.numberSamples, false, 10.0, DAQmx_Val_GroupByChannel, &voltages.mirror[0], NULL, NULL);
	// Write digital voltages
	DAQmxWriteDigitalLines(DOtaskHandle, voltages.numberSamples, false, 10, DAQmx_Val_GroupByChannel, &voltages.trigger[0], NULL, NULL);

	// Start DAQ tasks
	DAQmxStartTask(DOtaskHandle);
	DAQmxStartTask(AOtaskHandle);
}