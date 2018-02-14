#ifndef MATH_H
#define MATH_H

#include <vector>

class math {
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
};

#endif // MATH_H