#include "stdafx.h"
#include "Device.h"

Device::Device() {
}

Device::~Device() {
}

bool Device::getConnectionStatus() {
	return m_isConnected;
}