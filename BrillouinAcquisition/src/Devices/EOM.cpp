#include "stdafx.h" 
#include "EOM.h"
#include <cmath>
#include <iostream>

double EOM::frequencyToVoltage(double frequency)
{
    // Polynomial coefficients
    double p0 = 687.447;
    double p1 = -724.778;
    double p2 = 315.118;
    double p3 = -72.5507;
    double p4 = 9.36533;
    double p5 = -0.642672;
    double p6 = 0.0183256;

    // Polynomial (degree 6)
    double voltage = (p6 * std::pow(frequency, 6)) +
        (p5 * std::pow(frequency, 5)) +
        (p4 * std::pow(frequency, 4)) +
        (p3 * std::pow(frequency, 3)) +
        (p2 * std::pow(frequency, 2)) +
        (p1 * frequency) +
        p0;

    return std::round(voltage * 100.0) / 100.0;
}

void EOM::writeVoltage(const std::string& channel, double voltage) {
    TaskHandle taskHandle = nullptr;

    int32 error = DAQmxCreateTask("", &taskHandle);
    if (error) { std::cerr << "Error creating task: " << error << std::endl; return; }

    error = DAQmxCreateAOVoltageChan(taskHandle, channel.c_str(), "", 0.0, 10.0, DAQmx_Val_Volts, nullptr);
    if (error) { std::cerr << "Error creating channel: " << error << std::endl; DAQmxClearTask(taskHandle); return; }

    error = DAQmxWriteAnalogScalarF64(taskHandle, true, 10.0, voltage, nullptr);
    if (error) { std::cerr << "Error writing voltage: " << error << std::endl; }

    DAQmxStopTask(taskHandle);
    DAQmxClearTask(taskHandle);
}

void EOM::writeVoltageToDAQ(double voltage) {
    writeVoltage("Dev2/ao0", voltage);
}

void EOM::writeAttenuationVoltageToDAQ(double voltage) {
    writeVoltage("Dev2/ao1", voltage);
}