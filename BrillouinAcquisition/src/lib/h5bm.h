#ifndef H5BM_H
#define H5BM_H

#include <string>
#include <vector>
#include <bitset>
#include <QtWidgets>

#include "hdf5.h"
#include "TypesafeBitmask.h"
#include "..\..\src\Acquisition\AcquisitionModes\ScaleCalibrationHelper.h"
#include "..\..\src\Devices\Cameras\cameraParameters.h"

template <typename T>
struct IMAGE {
public:
	template <typename T>
	IMAGE(int indX, int indY, int indZ, int rank, hsize_t* dims, const std::string& date, const std::vector<T>& data,
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{}) :
		indX(indX), indY(indY), indZ(indZ), rank(rank), dims(dims), date(date), data(data), exposure(exposure), gain(gain), roi(roi) {};

	const int indX;
	const int indY;
	const int indZ;
	const int rank;
	const hsize_t *dims;
	const std::string date;
	const std::vector<T> data;
	const double exposure;
	const double gain;
	const CAMERA_ROI roi;
};

template <typename T>
struct CALIBRATION {
public:
	CALIBRATION(int index, const std::vector<T>& data, int rank, hsize_t *dims, const std::string& sample, double shift, const std::string& date,
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{}) :
		index(index), data(data), rank(rank), dims(dims), sample(sample), shift(shift), date(date), exposure(exposure), gain(gain), roi(roi) {};

	const int index;
	const std::vector<T> data;
	const int rank;
	const hsize_t *dims;
	const std::string sample;
	const double shift;
	const std::string date;
	const double exposure;
	const double gain;
	const CAMERA_ROI roi;
};

template <typename T>
struct ODTIMAGE {
public:
	ODTIMAGE(int ind, int rank, hsize_t *dims, const std::string& date, const std::vector<T>& data,
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{}) :
		ind(ind), rank(rank), dims(dims), date(date), data(data), exposure(exposure), gain(gain), roi(roi) {};

	const int ind;
	const int rank;
	const hsize_t *dims;
	const std::string date;
	const std::vector<T> data;
	const double exposure;
	const double gain;
	const CAMERA_ROI roi;
};

template <typename T>
struct FLUOIMAGE {
public:
	FLUOIMAGE(int ind, int rank, hsize_t *dims, const std::string& date, const std::string& channel, const std::vector<T>& data,
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{}) :
		ind(ind), rank(rank), dims(dims), date(date), channel(channel), data(data), exposure(exposure), gain(gain), roi(roi) {};

	const int ind;
	const int rank;
	const hsize_t *dims;
	const std::string date;
	const std::string channel;
	const std::vector<T> data;
	const double exposure;
	const double gain;
	const CAMERA_ROI roi;
};

struct ScaleCalibrationDataExtended : ScaleCalibrationData {
	POINT3 positionStage{ 0, 0, 0 };	// [micrometer] position of the stage
	POINT3 positionScanner{ 0, 0, 0 };	// [micrometer] position of the scanner
};

enum class ACQUISITION_MODE {
	NONE = 0x0,
	BRILLOUIN = 0x2,
	ODT = 0x4,
	FLUORESCENCE = 0x8,
	VOLTAGECALIBRATION = 0x10,
	SCALECALIBRATION = 0x12,
	MODECOUNT
};
ENABLE_BITMASK_OPERATORS(ACQUISITION_MODE)

struct RepetitionHandles {
	hid_t payload{ -1 };
	hid_t payloadData{ -1 };

	hid_t calibration{ -1 };
	hid_t calibrationData{ -1 };

	hid_t background{ -1 };
	hid_t backgroundData{ -1 };

public:
	RepetitionHandles(ACQUISITION_MODE mode, hid_t handle, bool create) { initialize(mode, handle, create); };
	~RepetitionHandles() {
		close();
	}
	void close() {
		closeGroup(calibrationData);
		closeGroup(calibration);
		closeGroup(backgroundData);
		closeGroup(background);
		closeGroup(payloadData);
		closeGroup(payload);
	}

private:
	void initialize(ACQUISITION_MODE mode, hid_t handle, bool create) {
		// payload handles
		payload = H5Gopen2(handle, "payload", H5P_DEFAULT);
		if (payload < 0 && create) {
			payload = H5Gcreate2(handle, "payload", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		}
		payloadData = H5Gopen2(payload, "data", H5P_DEFAULT);
		if (payloadData < 0 && create) {
			payloadData = H5Gcreate2(payload, "data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		}
		/*
		* Only Brillouin mode writes calibration and background data
		*/
		if ((bool)(mode & ACQUISITION_MODE::BRILLOUIN)) {
			// background handles
			background = H5Gopen2(handle, "background", H5P_DEFAULT);
			if (background < 0 && create) {
				background = H5Gcreate2(handle, "background", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			}
			backgroundData = H5Gopen2(background, "data", H5P_DEFAULT);
			if (backgroundData < 0 && create) {
				backgroundData = H5Gcreate2(background, "data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			}
			// calibration handles
			calibration = H5Gopen2(handle, "calibration", H5P_DEFAULT);
			if (calibration < 0 && create) {
				calibration = H5Gcreate2(handle, "calibration", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			}
			// legacy: this should actually be named "data" only
			calibrationData = H5Gopen2(calibration, "data", H5P_DEFAULT);
			if (calibrationData < 0 && create) {
				calibrationData = H5Gcreate2(calibration, "data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			}
		}
	}

	void closeGroup(hid_t& group) {
		if (group > -1) {
			if (H5Gclose(group) > -1) {
				group = -1;
			}
		}
	}
};

struct ModeHandles {
	ACQUISITION_MODE mode{ ACQUISITION_MODE::NONE };
	std::string modename{};
	hid_t rootHandle{ -1 };
	hid_t currentRepetitionHandle{ -1 };

	std::unique_ptr<RepetitionHandles> groups = nullptr;

	int repetitionCount{ 0 };

public:
	ModeHandles(ACQUISITION_MODE mode, const std::string& modename) : mode(mode), modename(modename) {};
	~ModeHandles() {
		if (groups) {
			groups->close();
		}
		close();
	};
	void close() {
		closeGroup(currentRepetitionHandle);
		closeGroup(rootHandle);
	}

private:
	void closeGroup(hid_t& group) {
		if (group > -1) {
			if (H5Gclose(group) > -1) {
				group = -1;
			}
		}
	}
};

class H5BM : public QObject {
	Q_OBJECT

public:
	H5BM(
		QObject *parent = 0,
		const std::string& filename = "Brillouin.h5",
		int flags = H5F_ACC_RDONLY
	) noexcept;
	~H5BM();

	ModeHandles* getModeHandle(ACQUISITION_MODE mode);
	void newRepetition(ACQUISITION_MODE mode);

	// date
	void setDate(const std::string& datestring);
	std::string getDate();

	// version
	// setVersion() is not implemented because the version attribute is set on file creation
	std::string getVersion();

	// comment
	void setComment(const std::string& comment);
	std::string getComment();

	// resolution
	void setResolution(std::string direction, int resolution);
	int getResolution(std::string direction);

	// scale calibration
	void setScaleCalibration(ACQUISITION_MODE mode, ScaleCalibrationDataExtended calibration);

	// positions
	void setPositions(std::string direction, const std::vector<double>& positions, const int rank, const hsize_t *dims);
	std::vector<double> getPositions(std::string direction);

	// payload data
	template <typename T>
	void setPayloadData(int indX, int indY, int indZ, const std::vector<T>& data, const int rank, const hsize_t *dims, const std::string& date = "now",
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{});

	template <typename T>
	void setPayloadData(IMAGE<T>*);
	template <typename T>
	void setPayloadData(ODTIMAGE<T>*);
	template <typename T>
	void setPayloadData(FLUOIMAGE<T>*);

	std::vector<double> getPayloadData(int indX, int indY, int indZ);
	std::string getPayloadDate(int indX, int indY, int indZ);

	// background data
	template <typename T>
	void setBackgroundData(const std::vector<T>& data, const int rank, const hsize_t *dims, const std::string& date = "now",
		double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{});
	std::vector<double> getBackgroundData();
	std::string getBackgroundDate();

	// calibration data
	template <typename T>
	void setCalibrationData(int index, const std::vector<T>& data, const int rank, const hsize_t *dims, const std::string& sample,
		double shift, const std::string& date = "now", double exposure = 0, double gain = 1, const CAMERA_ROI& roi = CAMERA_ROI{});
	std::vector<double> getCalibrationData(int index);
	std::string getCalibrationDate(int index);
	std::string getCalibrationSample(int index);
	double getCalibrationShift(int index);

private:
	bool m_fileWritable = false;
	bool m_fileValid = false;

	const std::string m_versionstring = "H5BM-v0.0.4";
	hid_t m_file{ -1 };		// handle to the opened file, default initialize to indicate no open file

	/*
	 *	Brillouin handles
	 */
	ModeHandles m_Brillouin{ ACQUISITION_MODE::BRILLOUIN, "Brillouin" };

	/*
	 *	ODT handles
	 */
	ModeHandles m_ODT{ ACQUISITION_MODE::ODT, "ODT" };

	/*
	 *	Fluorescence handles
	 */
	ModeHandles m_Fluorescence{ ACQUISITION_MODE::FLUORESCENCE, "Fluorescence" };

	void closeGroup(hid_t& group);
	void closeDataset(hid_t& dataset);

	template<class T>
	inline hid_t get_memtype();

#define HDF5_WRAPPER_SPECIALIZE_TYPE(T, tid) \
	template<> inline hid_t get_memtype<T>() { \
		return H5Tcopy(tid); \
	} \

	HDF5_WRAPPER_SPECIALIZE_TYPE(int, H5T_NATIVE_INT)
	HDF5_WRAPPER_SPECIALIZE_TYPE(unsigned int, H5T_NATIVE_UINT)
	HDF5_WRAPPER_SPECIALIZE_TYPE(unsigned short, H5T_NATIVE_USHORT)
	HDF5_WRAPPER_SPECIALIZE_TYPE(unsigned long long, H5T_NATIVE_ULLONG)
	HDF5_WRAPPER_SPECIALIZE_TYPE(long long, H5T_NATIVE_LLONG)
	HDF5_WRAPPER_SPECIALIZE_TYPE(char, H5T_NATIVE_CHAR)
	HDF5_WRAPPER_SPECIALIZE_TYPE(unsigned char, H5T_NATIVE_UCHAR)
	HDF5_WRAPPER_SPECIALIZE_TYPE(float, H5T_NATIVE_FLOAT)
	HDF5_WRAPPER_SPECIALIZE_TYPE(double, H5T_NATIVE_DOUBLE)
	HDF5_WRAPPER_SPECIALIZE_TYPE(bool, H5T_NATIVE_CHAR)
	HDF5_WRAPPER_SPECIALIZE_TYPE(unsigned long, H5T_NATIVE_ULONG)
	HDF5_WRAPPER_SPECIALIZE_TYPE(long, H5T_NATIVE_LONG)
	HDF5_WRAPPER_SPECIALIZE_TYPE(std::string, H5T_C_S1)

	void getRootHandle(ModeHandles& handle, bool create);

	void getRepetitionHandle(ModeHandles& handle, bool create);
	void getGroupHandles(ModeHandles& handle, bool create = false);

	void writePoint(hid_t group, const std::string& subGroupName, POINT2 point);

	// set/get attribute

	template<typename T>
	void setAttribute(const std::string& attrName, T* attrValue, hid_t parent, hid_t& type_id);
	void setAttribute(const std::string& attrName, const std::string& attr, hid_t parent);
	void setAttribute(const std::string& attrName, const char* attr, hid_t parent);
	template<typename T>
	void setAttribute(const std::string& attrName, T attr, hid_t parent);
	template<typename T>
	void setAttribute(const std::string& attrName, T attr);

	template<typename T>
	T getAttribute(std::string attrName, hid_t parent);
	template<>
	std::string getAttribute(std::string attrName, hid_t parent);
	template<typename T>
	T getAttribute(std::string attrName);

	template <typename T>
	hid_t setDataset(hid_t parent, std::vector<T> data, std::string name, const int rank, const hsize_t* dims);
	void getDataset(std::vector<double>* data, hid_t parent, std::string name);

	template <typename T>
	void setData(const std::vector<T>& data, const std::string& name, hid_t parent, const int rank, const hsize_t* dims,
		std::string date, const std::string& sample = "", double shift = NULL, const std::string& channel = "",
		double exposure = 0, double gain = 1, CAMERA_ROI roi = CAMERA_ROI{});

	std::vector<double> getData(const std::string& name, hid_t parent);
	std::string getDate(std::string name, hid_t parent);

	std::string calculateIndex(int indX, int indY, int indZ);

	std::string getNow();
};

template <typename T>
hid_t H5BM::setDataset(hid_t parent, std::vector<T> data, std::string name, const int rank, const hsize_t *dims) {
	hid_t type_id = get_memtype<T>();
	// For compatibility with MATLAB respect Fortran-style ordering: z, x, y
	hid_t space_id = H5Screate_simple(rank, dims, dims);

	hid_t dset_id;
	dset_id = H5Dopen2(parent, name.c_str(), H5P_DEFAULT);
	if (dset_id < 0) {
		dset_id = H5Dcreate2(parent, name.c_str(), type_id, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	}

	H5Dwrite(dset_id, get_memtype<T>(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());

	H5Sclose(space_id);
	H5Tclose(type_id);

	return dset_id;
}

template <typename T>
void H5BM::setData(const std::vector<T>& data, const std::string& name, hid_t parent, const int rank, const hsize_t *dims,
	std::string date, const std::string& sample, double shift, const std::string& channel, double exposure, double gain, CAMERA_ROI roi) {
	if (!m_fileWritable) {
		return;
	}

	if (date.compare("now") == 0) {
		date = getNow();
	}

	// write data
	hid_t dset_id = setDataset(parent, data, name, rank, dims);

	// write date
	setAttribute("date", date, dset_id);

	// write image attributes
	setAttribute("CLASS", "IMAGE", dset_id);
	setAttribute("IMAGE_VERSION", "1.2", dset_id);
	setAttribute("IMAGE_SUBCLASS", "IMAGE_GRAYSCALE", dset_id);

	// write sample name
	if (sample != "") {
		setAttribute("sample", sample, dset_id);
	}

	// write sample name
	if (shift != NULL) {
		setAttribute("shift", shift, dset_id);
	}

	// write channel name
	if (channel != "") {
		setAttribute("channel", channel, dset_id);
	}

	// set camera meta data
	setAttribute("exposure", exposure, dset_id);
	setAttribute("gain", gain, dset_id);

	// TODO: Should be replaced by a proper conversion from std::wstring to std::string
	auto binning = std::string{ "unknown" };
	if (roi.binning == L"1x1") {
		binning = "1x1";
	} else if (roi.binning == L"2x2") {
		binning = "2x2";
	} else if (roi.binning == L"4x4") {
		binning = "4x4";
	} else if (roi.binning == L"8x8") {
		binning = "8x8";
	}
	setAttribute("binning", binning, dset_id);
	setAttribute("ROI_left", (int)roi.left, dset_id);
	setAttribute("ROI_right", (int)roi.right, dset_id);
	setAttribute("ROI_top", (int)roi.top, dset_id);
	setAttribute("ROI_bottom", (int)roi.bottom, dset_id);
	setAttribute("ROI_height_physical", (int)roi.height_physical, dset_id);
	setAttribute("ROI_width_physical", (int)roi.width_physical, dset_id);
	setAttribute("ROI_height_binned", (int)roi.height_binned, dset_id);
	setAttribute("ROI_width_binned", (int)roi.width_binned, dset_id);

	closeDataset(dset_id);

	// write last-modified date to file
	setAttribute("last-modified", getNow());

	H5Fflush(m_file, H5F_SCOPE_GLOBAL);
}

template <typename T>
void H5BM::setPayloadData(int indX, int indY, int indZ, const std::vector<T>& data, const int rank, const hsize_t *dims, const std::string& date,
		double exposure, double gain, const CAMERA_ROI& roi) {
	auto name = calculateIndex(indX, indY, indZ);

	setData(data, name, m_Brillouin.groups->payloadData, rank, dims, date, "", NULL, "", exposure, gain, roi);
}

template <typename T>
void H5BM::setCalibrationData(int index, const std::vector<T>& data, const int rank, const hsize_t * dims, const std::string& sample,
	double shift, const std::string& date, double exposure, double gain, const CAMERA_ROI& roi) {
	setData(data, std::to_string(index), m_Brillouin.groups->calibrationData, rank, dims, date, sample, shift, "", exposure, gain, roi);
}

template <typename T>
void H5BM::setBackgroundData(const std::vector<T>& data, const int rank, const hsize_t *dims, const std::string& date,
	double exposure, double gain, const CAMERA_ROI& roi) {
	// legacy: this should actually be stored under "backgroundData"
	setData(data, "1", m_Brillouin.groups->background, rank, dims, date, "", NULL, "", exposure, gain, roi);
}

template <typename T>
void H5BM::setPayloadData(IMAGE<T>* image) {
	auto name = calculateIndex(image->indX, image->indY, image->indZ);

	setData(image->data, name, m_Brillouin.groups->payloadData, image->rank, image->dims, image->date,
		"", NULL, "", image->exposure, image->gain, image->roi);
}

template <typename T>
void H5BM::setPayloadData(ODTIMAGE<T>* image) {
	auto name = std::to_string(image->ind);

	setData(image->data, name, m_ODT.groups->payloadData, image->rank, image->dims, image->date,
		"", NULL, "", image->exposure, image->gain, image->roi);
}

template <typename T>
void H5BM::setPayloadData(FLUOIMAGE<T>* image) {
	auto name = std::to_string(image->ind);

	setData(image->data, name, m_Fluorescence.groups->payloadData, image->rank, image->dims, image->date, "", NULL, image->channel,
		image->exposure, image->gain, image->roi);
}

#endif // H5BM_H
