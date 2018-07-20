#include "stdafx.h"
#include "NIDAQ.h"

NIDAQ::NIDAQ() noexcept {
	m_availablePresets = {};
	m_availableElements = {};
}

NIDAQ::~NIDAQ() {
	disconnectDevice();
}

bool NIDAQ::connectDevice() {
	if (!m_isConnected) {

	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

bool NIDAQ::disconnectDevice() {
	if (m_isConnected) {
		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

void NIDAQ::init() {
}

void NIDAQ::setElement(ScanControl::DEVICE_ELEMENT element, int position) {
}

void NIDAQ::setElements(ScanControl::SCAN_PRESET preset) {
}

void NIDAQ::getElements() {
}

void NIDAQ::setPosition(std::vector<double> position) {
}

void NIDAQ::setPositionRelative(std::vector<double> distance) {
}

std::vector<double> NIDAQ::getPosition() {
	double x = 0;
	double y = 0;
	double z = 0;
	return std::vector<double> {x, y, z};
}