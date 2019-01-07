# CANControl

Matlab software used for controlling Zeiss LSM 510 META and Zeiss AxioVert 200M via Serial Port

## General

The devices are controlled via Serial communication. The correct code words to control them can be acquired with 'Free Serial Port Monitor' by sniffing the communication between 'CanCheck' or 'ZEN' and the devices to control. The already implemented functions are described in the next section.

The Matlab class for controlling the devices is named ```CANControl```.

## Devices and Functions

### Stand Axiovert 200:Focus

This device controls the position of the objective lens of the AxioVert 200M and is adressed using the prefix ```F```. You can get and set the current position and start and stop a continuous scan with a given velocity.

Commands:
- ```FPZp```	Request the current position. The read buffer has to be written out in order to get the position in hexadecimal units.
- ```FPZG1```	Set the velocity of the continuous scan as hexadecimal value. A value of ```3E8``` (1000) amounts to approximately 1.5 µm per second.
- ```FPZS-```	Start a scan in negative direction.
- ```FPZS+```	Start a scan in positive direction.
- ```FPZSS```	Stop the scan.
- ```FPZDxxxxxx```	Set the current position with ```xxxxxx``` as the desired position in hexadecimal units. One increment corresponds to 0.025 µm.
- ```FPZt```	State of scanner. ```0``` corresponds to static focus, ```255``` means moving lens.
- ```FPw```		Status key of the current position. ```01``` corresponds to the work position, ```04``` means load position.
- ```FPW0```	Moves the focus to the load position.
- ```FPW1```	Moves the focus to the work position.

### Stand Axiovert 200:Stand

This device controls the objective, reflector, sideport etc. of the AxioVert 200M and is adressed using the prefix ```H```. You can set the positions of the devices.

Commands:
- ```HPCR1,x```		Set the position of the Reflector. The value ```x``` is the desired position with values ```[1, 2, 3, 4, 5]```.
- ```HPCr1,1```		Get the position of the Reflector.  Returns ```PHx``` with ```x``` as the position of the Reflector. A value of ```0``` means moving element.
- ```HPCR2,x```		Set the position of the Objective. The value ```x``` is the desired position with values ```[1, 2, 3, 4, 5, 6]```.
- ```HPCr2,1```		Get the position of the Objective.  Returns ```PHx``` with ```x``` as the position of the Objective. A value of ```0``` means moving element.
- ```HPCR36,x```	Set the position of the Tubelens. The value ```x``` is the desired position with values ```[1, 2, 3]```.
- ```HPCr36,1```	Get the position of the Tubelens.  Returns ```PHx``` with ```x``` as the position of the Tubelens. A value of ```0``` means moving element.
- ```HPCR38,x```	Set the position of the Baseport. The value ```x``` is the desired position with values ```[1, 2, 3]```.
- ```HPCr38,1```	Get the position of the Baseport.  Returns ```PHx``` with ```x``` as the position of the Baseport. A value of ```0``` means moving element.
- ```HPCR39,x```	Set the position of the Sideport. The value ```x``` is the desired position with values ```[1, 2, 3]```.
- ```HPCr39,1```	Get the position of the Sideport.  Returns ```PHx``` with ```x``` as the position of the Sideport. A value of ```0``` means moving element.
- ```HPCR51,x```	Set the position of the Mirror. The value ```x``` is the desired position with values ```[1, 2]```.
- ```HPCr51,1```	Get the position of the Mirror.  Returns ```PHx``` with ```x``` as the position of the Mirror A value of ```0``` means moving element.

There are more commands which are not necessary for the Axiovert 200M in Room 340.

### LSM 510:Scanhead

This device controls the laser scanning module and is adressed using the prefix ```I```. You can control beam splitters, collimators, filters and pinholes.


#### Beam splitters

Commands:
- ```IPCr6,1```		Get the position of the main beam splitter (HT). Returns ```PIxxx``` with ```xxx``` as the position of HT.
- ```IPCR6,x```		Set the position of HT. The value ```x``` is the desired position with values ```[25, 19, 13, 7, 1, 43, 37, 31]``` corresponding to the positions ```[1, 2, 3, 4, 5, 6, 7, 8]``` of HT.
- ```IPCr7,1```		Get the position of the secondary beam splitter 1 (NT1). Returns ```PIxxx``` with ```xxx`` as the position of NT1.
- ```IPCR7,x```		Set the position of NT1. The value ```x``` is the desired position with values ```[25, 19, 13, 7, 1, 43, 37, 31]``` corresponding to the positions ```[1, 2, 3, 4, 5, 6, 7, 8]``` of NT1.
- ```IPCr8,1```		Get the position of the secondary beam splitter 2 (NT2). Returns ```PIxxx``` with ```xxx``` as the position of NT2.
- ```IPCR8,x```		Set the position of NT2. The value ```x``` is the desired position with values ```[10, 1, 28, 19]``` corresponding to the positions ```[1, 2, 3, 4]``` of NT2.
- ```IPCr9,1```		Get the position of the secondary beam splitter 3 (NT3). Returns ```PIxxx``` with ```xxx``` as the position of NT3.
- ```IPCR9,x```		Set the position of NT3. The value ```x``` is the desired position with values ```[10, 1, 28, 19]``` corresponding to the positions ```[1, 2, 3, 4]``` of NT3.


#### Filters

Commands:
- ```IPCr11,1```	Get the position of filter wheel 2 (EF2). Returns ```PIxxx``` with ```xxx``` as the position of EF2.
- ```IPCR11,x```	Set the position of EF2. The value ```x``` is the desired position with values ```[25, 31, 37, 43, 1, 7, 13, 19]``` corresponding to the positions ```[1, 2, 3, 4, 5, 6, 7, 8]``` of EF2.
- ```IPCr12,1```	Get the position of filter wheel 3 (EF3). Returns ```PIxxx``` with ```xxx``` as the position of EF3.
- ```IPCR12,x```	Set the position of EF3. The value ```x``` is the desired position with values ```[18, 24, 30, 36, 42, 48, 6, 12, 18]``` corresponding to the positions ```[1, 2, 3, 4, 5, 6, 7, 8]``` of EF3.
- ```IPCr14,1```	Get the position of filter wheel 5 (EF5). Returns ```PIxxx``` with ```xxx``` as the position of EF5.
- ```IPCR14,x```	Set the position of EF5. The value ```x``` is the desired position with values ```[1, 7, 13, 19, 25, 31, 37, 43]``` corresponding to the positions ```[1, 2, 3, 4, 5, 6, 7, 8]``` of EF5.


#### Pinholes

Commands:
- ```IPCs16,1```	Get the x-position of pinhole 1 (PH1). Returns ```PIxxx``` with ```xxx``` as the position of PH1.
- ```IPCS16,x```	Set the x-position of PH1. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs20,1```	Get the y-position of PH1. Returns ```PIxxx``` with ```xxx``` as the position of PH1.
- ```IPCS20,x```	Set the y-position of PH1. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs24,1```	Get the z-position of PH1. Returns ```PIxxx``` with ```xxx``` as the position of PH1.
- ```IPCS24,x```	Set the z-position of PH1. The value ```x``` is the desired position with values from ```1``` to ```36```.
- ```IPCs27,1```	Get the diameter of PH1. Returns ```PIxxx``` with ```xxx``` as the diameter of PH1.
- ```IPCS27,x```	Set the diameter of PH1. The value ```x``` is the desired diameter with values from ```1``` to ```250```.

- ```IPCs17,1```	Get the x-position of pinhole 2 (PH2). Returns ```PIxxx``` with ```xxx``` as the position of PH2.
- ```IPCS17,x```	Set the x-position of PH2. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs21,1```	Get the y-position of PH2. Returns ```PIxxx``` with ```xxx``` as the position of PH2.
- ```IPCS21,x```	Set the y-position of PH2. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs28,1```	Get the diameter of PH2. Returns ```PIxxx``` with ```xxx``` as the diameter of PH2.
- ```IPCS28,x```	Set the diameter of PH2. The value ```x``` is the desired diameter with values from ```1``` to ```250```.

- ```IPCs18,1```	Get the x-position of pinhole 3 (PH3). Returns ```PIxxx``` with ```xxx``` as the position of PH3.
- ```IPCS18,x```	Set the x-position of PH3. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs22,1```	Get the y-position of PH3. Returns ```PIxxx``` with ```xxx``` as the position of PH3.
- ```IPCS22,x```	Set the y-position of PH3. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs29,1```	Get the diameter of PH3. Returns ```PIxxx``` with ```xxx``` as the diameter of PH3.
- ```IPCS29,x```	Set the diameter of PH3. The value ```x``` is the desired diameter with values from ```1``` to ```250```.

- ```IPCs19,1```	Get the x-position of pinhole 4 (PH4). Returns ```PIxxx``` with ```xxx``` as the position of PH4.
- ```IPCS19,x```	Set the x-position of PH4. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs23,1```	Get the y-position of PH4. Returns ```PIxxx``` with ```xxx``` as the position of PH4.
- ```IPCS23,x```	Set the y-position of PH4. The value ```x``` is the desired position with values from ```1``` to ```250```.
- ```IPCs30,1```	Get the diameter of PH4. Returns ```PIxxx``` with ```xxx``` as the diameter of PH4.
- ```IPCS30,x```	Set the diameter of PH4. The value ```x``` is the desired diameter with values from ```1``` to ```250```.


#### Collimators

Commands:
- ```IPCs25,1```	Get the position of collimator 1. Returns ```PIxxx``` with ```xxx``` as the position of collimator 1.
- ```IPCS25,x```	Set the position of collimator 1. The value ```x``` is the desired position with values from ```1```to ```120```.
- ```IPCs26,1```	Get the position of collimator 2. Returns ```PIxxx``` with ```xxx``` as the position of collimator 2.
- ```IPCS26,x```	Set the position of collimator 2. The value ```x``` is the desired position with values from ```1```to ```110```.

### LSM 510:Meta

This device controls the META detector and is adressed using the prefix ```C```. You can control the grating position.

Commands:
- ```CPCs26,1```	Get the position of the grating 1 (GS1). Returns ```PCxxx``` with ```xxx``` as the position of GS1.
- ```CPCS26,x```	Set the position of GS1. The value ```x``` is the desired position with values from ```1``` to ```600```.

### Stage 28:MCU 28

This device controls the x- and y-position of the translation stage and is adressed using the prefix ```N```.

Commands:
- ```NPXp```		Get the x-position. Returns ```PNxxxxxx``` with ```xxxxxx``` as the x-position of the stage.
- ```NPYp```		Get the y-position. Returns ```PNxxxxxx``` with ```xxxxxx``` as the y-position of the stage.
- ```NPXS```		Stops the movement of the x-axis.
- ```NPYS```		Stops the movement of the y-axis.
- ```NPXVx```		Sets the velocity of the x-axis to the decimal value ```x```. A value of 1 corresponds to a velocity of approximately 500 µm per second.
- ```NPYVx```		Sets the velocity of the y-axis to the decimal value ```x```. A value of 1 corresponds to a velocity of approximately 500 µm per second.
- ```NPXt```		State of x-axis. ```0``` corresponds to a static stage, ```255``` means moving stage.
- ```NPYt```		State of y-axis. ```0``` corresponds to a static stage, ```255``` means moving stage.
- ```NPXmx```		Purpose unknown. Accepts value from 0 to 9 for ```x```, returns either 0 or 1.
- ```NPYmx```		Purpose unknown. Accepts value from 0 to 9 for ```x```, returns either 0 or 1.
- ```NPXTxxxxxx```	Set the x-position of the stage. The value ```xxxxxx``` is the desired position.
- ```NPYTxxxxxx```	Set the y-position of the stage. The value ```xxxxxx``` is the desired position.
