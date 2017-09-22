#pragma once
#include "atcore.h"

class Andor {

private:
	AT_H Hndl;
	bool initialised = FALSE;
	bool connected = FALSE;

public:
	Andor() {
		int i_retCode;
		i_retCode = AT_InitialiseLibrary();
		if (i_retCode != AT_SUCCESS) {
			//error condition, check atdebug.log file
		} else {
			initialised = TRUE;
		}
	}

	~Andor() {
		if (connected) {
			AT_Close(Hndl);
		}
		AT_FinaliseLibrary();
	}

	void checkCamera();
};
