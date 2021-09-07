# BrillouinAcquisition

**BrillouinAcquisition** is used to acquire Brillouin microscopy data as well as optical diffraction tomography and fluorescence imaging data from custom build Brillouin and FOB microscopy devices.

## 1. System requirements

The software only runs on Microsoft Windows. To build the program from the source code, Visual Studio is required.


## 2. Installation guide

Since the software requires multiple proprietary libraries to be build and run, no pre-built packages can be distributed. Please build the software from the provided source files or get in contact with the developers.

### Building the software

Clone the BrillouinAcquisition repository using `git clone` and install the required submodules with `git submodule init` and `git submodule update`.

Please install the following dependencies to the default paths:

- Qt 5.15.1 or higher
- Thorlabs Kinesis
- National Instruments NI-DAQmx
- Point Grey Research FlyCapture2
- IDS uEye Software Suite
- Andor Imaging SDK
- HDF Group HDF5 library
- Photometrics PVCam SDK
- Carl Zeiss MTB 2011 SDK
- OpenCV

Build the BrillouinAcquisition project using Visual Studio.