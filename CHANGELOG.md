## 0.3.1 - 2022-12-05

### Changed
- Dependency updates #225 #226 #228 #229
- Code cleanup #223 #224 #227 #230

## 0.3.0 - 2022-09-05

### Added
- Allow to chose camera number #215

### Changed
- Move relative when clicking into image #220
- Code cleanup #216

### Fixed
- Fix crash for scale calibration without camera #213
- Don't crash if calibration file cannot be written #214

## 0.2.2 - 2021-08-12

### Fixed
- Fix connectivity to Thorlabs devices #206

## 0.2.1 - 2021-08-10

### Fixed
- Correct position preview after aborting a measurement #198
- Adjust preview settings #202
- Correct mirror positions for FOB setup #200
- Fix calculation of bytes per frame on Andor #199

## 0.2.0 - 2021-03-03

### Added
- Show measurement positions as overlay in brightfield image #131 #141
- Implement acquiring a scale calibration #124 #155
- Store scale calibration in acquisition file #124 #149
- Automatically load last scale calibration  #189 #190
- Store camera meta data in acquisition file #107 #138 #192
- Allow to create separate data files per repetition #169 #170
- Implement RLShutter and lamp control for ZeissECU #180 #186
- Adjust colormaps used #126 #140
- Ask for confirmation before closing #150 #151
- Show version in title bar #154
- Add mock camera to simplify debugging #158 #163 
- Add license
- Add readme

### Fixed
- Correctly handle binning in combination with repetitions #142 #144 #152 #153
- Correctly handle odd image sizes when binning #145 #146 #147
- Block the laser when in eyepiece mode #185 #188
- Correctly close handles to HDF5 objects #168
- Save camera data as correct type #157 #174
- Acquisition sometimes crashes during voltage calibration #127 #141
- Fix high CPU load after brightfield/fluorescence acquisition #171
- Correctly delete variables #176 #177
- Reduce CPU load for camera preview by using signals #178 #179 #187
- Correctly exit threads #172
- Code cleanup #148 #181 #182 #183 #184

## 0.1.0 - 2020-11-02

### Added
- Add support for binning on Andor and PVCam cameras
- Add changelog