#ifndef SIMPLEMATH_H
#define SIMPLEMATH_H

#include <vector>
#include <complex>

struct COEFFICIANTS5 {
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
	static std::vector<std::complex<T>> solveQuartic(COEFFICIANTS5 coef) {
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
};

#endif // SIMPLEMATH_H