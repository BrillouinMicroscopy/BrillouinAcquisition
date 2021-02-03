#ifndef SIMPLEMATH_H
#define SIMPLEMATH_H

#include <vector>
#include <iterator>
#include <complex>

struct COEFFICIENTS5 {
	double a{ 0 };		// coefficient of fourth order
	double b{ 0 };		// coefficient of third order
	double c{ 0 };		// coefficient of second order
	double d{ 0 };		// coefficient of first order
	double e{ 0 };		// offset term
};

class simplemath {
public:
	// return linearly spaced vector
	template <typename T = double>
	static std::vector<T> linspace(T min, T max, size_t N) {
		T spacing = (max - min) / static_cast<T>(N - 1);
		std::vector<T> xs(N);
		typename std::vector<T>::iterator x;
		T value;
		for (x = xs.begin(), value = min; x != xs.end(); ++x, value += spacing) {
			*x = value;
		}
		return xs;
	}

	template <typename T = double>
	static T min(std::vector<T> vector) {
		std::vector<T>::iterator result = std::min_element(std::begin(vector), std::end(vector));
		return *result;
	}

	template <typename T = double>
	static T max(std::vector<T> vector) {
		std::vector<T>::iterator result = std::max_element(std::begin(vector), std::end(vector));
		return *result;
	}
	
	template <typename T = double>
	static T absSum(std::vector<T> v) {
		return std::accumulate(v.begin(), v.end(), (T) 0, [](T acc, T val) {
			return acc + abs(val);
		});
	}

	template <typename T = double>
	static std::vector<std::complex<T>> solveQuartic(COEFFICIENTS5 coef) {
		// shorthands for the variables
		double a = coef.a;
		double b = coef.b;
		double c = coef.c;
		double d = coef.d;
		double e = coef.e;

		// intermediate variables
		std::complex<T> p1 = 2 * pow(c, 3) - 9 * b*c*d + 27 * a*pow(d, 2) + 27 * pow(b, 2) * e - 72 * a*c*e;
		std::complex<T> p2 = p1 + sqrt(-4 * pow((pow(c, 2) - 3 * b*d + 12 * a*e), 3) + pow(p1, 2));
		std::complex<T> p3 = (pow(c, 2) - 3.0 * b*d + 12.0 * a*e) / (3.0 * a*pow((p2 / 2.0), (1.0 / 3.0))) + pow((p2 / 2.0), (1.0 / 3.0)) / (3.0 * a);

		std::complex<T> p4 = sqrt(pow(b, 2) / (4.0 * pow(a, 2)) - (2.0 * c) / (3.0 * a) + p3);
		std::complex<T> p5 = pow(b, 2) / (2 * pow(a, 2)) - (4.0 * c) / (3.0 * a) - p3;
		std::complex<T> p6 = (-1.0 * (pow(b, 3)) / pow(a, 3) + (4.0 * b*c) / pow(a, 2) - (8.0 * d) / a) / (4.0 * p4);

		std::vector<std::complex<T>> solutions(4);
		solutions[0] = -b / (4.0 * a) - p4 / 2.0 - sqrt(p5 - p6) / 2.0;
		solutions[1] = -b / (4.0 * a) - p4 / 2.0 + sqrt(p5 - p6) / 2.0;
		solutions[2] = -b / (4.0 * a) + p4 / 2.0 - sqrt(p5 + p6) / 2.0;
		solutions[3] = -b / (4.0 * a) + p4 / 2.0 + sqrt(p5 + p6) / 2.0;
		return solutions;
	}

	template <typename T>
	static int sgn(T val) {
		return (T(0) < val) - (val < T(0));
	}

	template<typename T>
	static bool contains(std::vector<T> & vector, const T& value) {
		if (std::find(vector.begin(), vector.end(), value) != vector.end()) {
			return true;
		}
		return false;
	}

	// See https://stackoverflow.com/questions/37732275/how-to-get-the-sorted-index-of-a-vector
	template<typename T>
	static std::vector<std::size_t> tag_sort_inverse(const std::vector<T>& v) {
		std::vector<std::size_t> result(v.size());
		std::iota(std::begin(result), std::end(result), 0);
		std::sort(std::begin(result), std::end(result),
			[&v](const auto & lhs, const auto & rhs) {
				return v[lhs] > v[rhs];
			}
		);
		return result;
	}

	// Function to calculate the median value of a vector
	// See https://stackoverflow.com/questions/1719070/what-is-the-right-approach-when-using-stl-container-for-median-calculation/2579393#2579393

	// Exception to throw when vector handed to median function is empty
	class median_of_empty_list_exception :public std::exception {
		virtual const char* what() const throw() {
			return "Attempt to take the median of an empty list of numbers.  "
				"The median of an empty list is undefined.";
		}
	};

	/* 
	 * Return the median of a sequence of numbers given by the random
	 * access iterators begin and end.
	 *
	 * The sequence must not be empty (median is undefined for an empty set).
	 * The numbers must be convertible to double.
	 */
	template<class It>
	static double median(It begin, It end) throw(median_of_empty_list_exception) {
		if (begin == end) {
			throw median_of_empty_list_exception();
		}
		std::size_t size = end - begin;
		std::size_t middleIdx = size / 2;
		It target = begin + middleIdx;
		std::nth_element(begin, target, end);

		if (size % 2 != 0) {	// Odd number of elements
			return *target;
		} else {				// Even number of elements
			double a = *target;
			It targetNeighbor = target - 1;
			std::nth_element(begin, targetNeighbor, end);
			return (a + *targetNeighbor) / 2.0;
		}
	}

	template<typename T>
	static T sum(const std::vector<T>& v) {
		return std::accumulate(v.begin(), v.end(), (T) 0);
	}
};

#endif // SIMPLEMATH_H