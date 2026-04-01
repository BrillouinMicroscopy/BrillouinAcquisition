#ifndef EOM_H
#define EOM_H

#include <NIDAQmx.h>

class EOM {
public:
    static void writeVoltageToDAQ(double voltage);
    static void writeAttenuationVoltageToDAQ(double voltage);
    static double frequencyToVoltage(double frequency);

private:
    static void writeVoltage(const std::string& channel, double voltage);
};
#endif // EOM_H
