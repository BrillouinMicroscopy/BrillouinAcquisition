#ifndef BITMASK_H
#define BITMASK_H

/*
 * Implemented following this blogpost: http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
 */

template<typename Enum>
struct EnableBitMaskOperators {
	static const bool enable{ false };
};

#define ENABLE_BITMASK_OPERATORS(x)	\
template<>							\
struct EnableBitMaskOperators<x> {	\
	static const bool enable{ true };\
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator |(T lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	return static_cast<T> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
	);
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator &(T lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	return static_cast<T> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
	);
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator ^(T lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	return static_cast<T> (
		static_cast<underlying>(lhs) ^
		static_cast<underlying>(rhs)
	);
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type
operator ~(T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	return static_cast<T> (
		~static_cast<underlying>(rhs)
	);
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type&
operator |=(T &lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	lhs = static_cast<T> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
	);
	return lhs;
}

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type&
operator &=(T &lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	lhs = static_cast<T> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
	);
	return lhs;
};

template<typename T>
typename std::enable_if<EnableBitMaskOperators<T>::enable, T>::type&
operator ^=(T &lhs, T rhs) {
	using underlying = typename std::underlying_type<T>::type;
	lhs = static_cast<T> (
		static_cast<underlying>(lhs) ^
		static_cast<underlying>(rhs)
	);
	return lhs;
};

#endif // BITMASK_H