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

## 3. Compilation

### Project structure

```bash
BrillouinAcquisition/
│
├── BrillouinAcquisition/
│   └── ProBrillouinAcquisitionject.vcxproj        # Main application project
│
├── BrillouinAcquisitionUnitTest/
│   └── BrillouinAcquisitionUnitTest.vcxproj       # Unit test project
|
├── CommonProps/
│   └── Thirdparty.props       # Centralized property definitions
│
└── x64/
    ├── Debug/
    └── Release/               # Output directories (TargetDir)

```

### Configure the `.props` File  
- Open `CommonProps/Thirdparty.props` file

- Update the hardcoded paths (e.g., QtInstallDir, OpenCV, Andor SDK paths) to match your local machine.
    ```xml
    <QTDIR>C:\Qt\5.15.x\5.15.2\msvc2019_64</QTDIR>
    <ANDOR_SDK_DIR>C:\Program Files\Andor SDK3</ANDOR_SDK_DIR>
    <HDF5_DIR>C:\Program Files\HDF_Group\HDF5\1.12.1</HDF5_DIR>
    <THORLABS_DIR>C:\Program Files\Thorlabs\Kinesis</THORLABS_DIR>
    <NI_EXTCOMPILER_DIR>C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C</NI_EXTCOMPILER_DIR>
    <IDS_UEYE_DIR>c:\Program Files\IDS\uEye</IDS_UEYE_DIR>
    <POINT_GREY_DIR>C:\Program Files\Point Grey Research\FlyCapture2</POINT_GREY_DIR>
    <PHOTOMETRICS_PVCAM_DIR>c:\Program Files\Photometrics\PVCamSDK</PHOTOMETRICS_PVCAM_DIR>
    <CARL_ZEISS_MTB_DIR>C:\Program Files\Carl Zeiss\MTB 2011 - 2.15.0.2</CARL_ZEISS_MTB_DIR>
    <OPENCV_DIR>C:\Users\ralajan\Downloads\BrillouinMicroscopy\OpenCV\470\opencv\build</OPENCV_DIR>
    ```
- Make sure you set the QtInstallDir and QtVersion in `Thirdparty.props` file.
    ```xml
    <PropertyGroup Label="QtSettings">
    <QtInstallDir>C:\Qt\5.15.x\5.15.2\msvc2019_64</QtInstallDir>
    <QtVersion>qt_5_15_2</QtVersion>
    <QtModules>core;gui;widgets;serialport;printsupport</QtModules>
  </PropertyGroup>
    ```
- No need to change TargetDir — it dynamically adjusts to Debug/Release builds.

### Open the Solution in Visual Studio
- Open the `.sln` file or directly open the `.vcxproj` in Visual Studio.

- Set the configuration to either Debug or Release.

- Build the project (`ctrl + shift + B`).

### What the `.props` File Does
- Centralizes all 3rd-party include paths and `.lib` dependencies.

- Adds build-specific (debug/release) settings and libraries.

- Automatically copies required DLLs after build (PostBuildCommand) to x64\Debug\ or x64\Release\.

- Uses `windeployqt` to copy necessary Qt runtime files.

- Debug vs Release differences (e.g., OpenCV d vs normal libs)


### Troubleshooting
- DLL Not Found? - Check that the paths in `Thirdparty.props` are valid and that the required `.dll` files exist.

- Build errors? - Confirm that Visual Studio uses the correct platform toolset (MSVC v142).

- Visual differences between Debug & Release? - Ensure consistent runtime libraries and DLLs are being deployed (e.g., fonts/styles in Qt).

# How to cite

If you use BrillouinAcquisition in a scientific publication, please cite it with:

> Raimund Schlüßler and others (2017), BrillouinAcquisition version X.X.X: C++ program for the acquisition of FOB microscopy data sets [Software]. Available at https://github.com/BrillouinMicroscopy/BrillouinAcquisition.

If the journal does not accept ``and others``, you can fill in the missing
names from the [credits file](https://github.com/BrillouinMicroscopy/BrillouinAcquisition/blob/master/CREDITS).