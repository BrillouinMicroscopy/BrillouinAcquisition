#include "stdafx.h"
#include "scancontrol.h"

bool ScanControl::getConnectionStatus() {
	return m_isConnected + m_isCompatible;
}