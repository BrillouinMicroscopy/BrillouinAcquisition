#include "stdafx.h"
#include "scancontrol.h"
#include <iomanip>
#include <sstream>

ScanControl::ScanControl() {
	focus = new Focus(m_comObject);
}

ScanControl::~ScanControl() {
	delete focus;
	disconnect();
}

void ScanControl::connect() {

}

void ScanControl::disconnect() {

}

void ScanControl::setPosition(std::vector<double> position) {
	mcu->setX(position[0]);
	mcu->setY(position[1]);
	focus->setZ(position[2]);
}

std::vector<double> ScanControl::getPosition() {
	double x = mcu->getX();
	double y = mcu->getY();
	double z = focus->getZ();
	return std::vector<double> {x, y, z};
}

ScanControl::Focus::Focus(com *comObject) : Element(comObject, m_prefix) {}

double ScanControl::Focus::getZ() {
	std::string s = "0x" + receive("Zp");
	return m_umperinc * hex2dec(s);
}

void ScanControl::Focus::setZ(double position) {
}

ScanControl::Element::~Element() {
}

std::string ScanControl::Element::receive(std::string request) {
	return m_comObject->receive(m_prefix + request);
}

std::string ScanControl::Element::dec2hex(int dec) {
	std::stringstream stream;
	stream << std::hex << dec;
	return stream.str();
}

int ScanControl::Element::hex2dec(std::string s) {
	return std::stoul(s, nullptr, 16);
}

ScanControl::MCU::MCU(com *comObject) : Element(comObject, m_prefix) {
}

double ScanControl::MCU::getX() {
	return 0.0;
}

void ScanControl::MCU::setX(double position) {
}

double ScanControl::MCU::getY() {
	return 0.0;
}

void ScanControl::MCU::setY(double position) {
}

std::string ScanControl::com::receive(std::string request) {
	return std::string();
}
