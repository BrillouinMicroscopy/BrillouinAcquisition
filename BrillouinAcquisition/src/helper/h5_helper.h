#ifndef H5_HELPER
#define H5_HELPER

#include "H5Cpp.h"

class h5_helper {
public:
	template<class T>
	static inline H5::PredType get_memtype();

#define H5_WRAPPER_SPECIALIZE_TYPE(T, tid) \
	template<> static inline H5::PredType get_memtype<T>() { \
		return tid; \
	} \

	H5_WRAPPER_SPECIALIZE_TYPE(int, H5::PredType::NATIVE_INT)
	H5_WRAPPER_SPECIALIZE_TYPE(unsigned int, H5::PredType::NATIVE_UINT)
	H5_WRAPPER_SPECIALIZE_TYPE(unsigned short, H5::PredType::NATIVE_USHORT)
	H5_WRAPPER_SPECIALIZE_TYPE(unsigned long long, H5::PredType::NATIVE_ULLONG)
	H5_WRAPPER_SPECIALIZE_TYPE(long long, H5::PredType::NATIVE_LLONG)
	H5_WRAPPER_SPECIALIZE_TYPE(char, H5::PredType::NATIVE_CHAR)
	H5_WRAPPER_SPECIALIZE_TYPE(unsigned char, H5::PredType::NATIVE_UCHAR)
	H5_WRAPPER_SPECIALIZE_TYPE(float, H5::PredType::NATIVE_FLOAT)
	H5_WRAPPER_SPECIALIZE_TYPE(double, H5::PredType::NATIVE_DOUBLE)
	H5_WRAPPER_SPECIALIZE_TYPE(bool, H5::PredType::NATIVE_CHAR)
	H5_WRAPPER_SPECIALIZE_TYPE(unsigned long, H5::PredType::NATIVE_ULONG)
	H5_WRAPPER_SPECIALIZE_TYPE(long, H5::PredType::NATIVE_LONG)
};

#endif // H5_HELPER