#ifndef PHASE_H
#define PHASE_H

#include <vector>
#include <iterator>
#include <complex>
#include <utility>
#include <iterator>

#include <math.h>
#include "simplemath.h"

#include "../external/fftw/fftw3.h"
#include "unwrap2Wrapper.h"
#include "xsample.h"

class phase {

private:

	fftw_complex *m_background = nullptr;
	fftw_complex *m_in_FFT, *m_out_FFT, *m_in_IFFT, *m_out_IFFT;
	fftw_plan m_FFT, m_IFFT;
	int m_dim_x{ 0 }, m_dim_y{ 0 }, m_max_x{ 0 }, m_max_y{ 0 }, m_dim_background_x{ 0 }, m_dim_background_y{ 0 };
	bool m_initialized{ false };

	double m_pixelSize = 4.8 / (90.4762 * 63 / 100);
	double m_NA{ 1.2 };
	double m_lambda{ 0.532 };

	double m_maskRadius{ 0 };
	std::vector<int> m_mask;

	unwrap2Wrapper *m_unwrapper = new unwrap2Wrapper();
	xsample *m_xsample = new xsample();

	template <typename T = double>
	bool sizeMatches(T intensity) {
		return false;
	}

	/*
	 * We have to (re-)initialize the FFT calculation whenever the image size changes
	 */
	void initialize(int dim_x, int dim_y) {
		if (m_dim_x != dim_x || m_dim_y != dim_y) {
			m_dim_x = dim_x;
			m_dim_y = dim_y;

			m_maskRadius = round(m_dim_x * m_pixelSize * m_NA / m_lambda);

			m_mask = createMask(dim_x, dim_y, m_maskRadius);

			int N = m_dim_x * m_dim_y;

			if (m_initialized) {
				// FFT variables
				fftw_free(m_in_FFT);
				fftw_free(m_out_FFT);
				fftw_destroy_plan(m_FFT);

				// Inverse FFT variables
				fftw_free(m_in_IFFT);
				fftw_free(m_out_IFFT);
				fftw_destroy_plan(m_IFFT);
			}

			m_in_FFT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_out_FFT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_in_IFFT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_out_IFFT = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_FFT = fftw_plan_dft_2d(m_dim_y, m_dim_x, m_in_FFT, m_out_FFT, FFTW_FORWARD, FFTW_ESTIMATE);
			m_IFFT = fftw_plan_dft_2d(m_dim_y, m_dim_x, m_in_IFFT, m_out_IFFT, FFTW_BACKWARD, FFTW_ESTIMATE);

			m_initialized = true;
		}
	}

	std::vector<int> createMask(int dim_x, int dim_y, double maskRadius) {
		std::vector<int> mask(dim_x * dim_y, 0);
		for (int x{ (int)round(dim_x / 2.0 - m_maskRadius) }; x < round(dim_x / 2.0 + m_maskRadius); x++) {
			for (int y{ (int)round(dim_y / 2.0 - m_maskRadius) }; y < round(dim_y / 2.0 + m_maskRadius); y++) {
				if (sqrt(pow((x - dim_x/2.0), 2) + pow(y - dim_y/2.0, 2)) <= m_maskRadius) {
					mask[x + dim_x * y] = 1;
				}
			}
		}

		return mask;
	}

	template <typename T_in = double>
	void copyToInput(fftw_complex* dest, T_in* intensity, int dim_x, int dim_y) {
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			dest[i][0] = intensity[i];
			dest[i][1] = 0.0;
		}
	}

	void getRawPhase() {
		// Shift background spectrum to center and mask unwanted regions
		circshift(m_out_FFT, m_dim_x, m_dim_y, -m_max_x, -m_max_y);
		for (int jj{ 0 }; jj < m_dim_x * m_dim_y; jj++) {
			m_out_FFT[jj][0] *= m_mask[jj];
			m_out_FFT[jj][1] *= m_mask[jj];
		}

		// Calculate inverse Fourier transform of shifted and masked background
		int N = m_dim_x * m_dim_y;
		memcpy(m_in_IFFT, m_out_FFT, sizeof(fftw_complex) * N);

		fftw_execute(m_IFFT);
	}

public:

	bool m_updateBackground{ false };

	phase() {}

	~phase() {
		fftw_destroy_plan(m_FFT);
		fftw_free(m_in_FFT);
		fftw_free(m_out_FFT);

		fftw_destroy_plan(m_IFFT);
		fftw_free(m_in_IFFT);
		fftw_free(m_out_IFFT);

		if (m_background != nullptr) {
			fftw_free(m_background);
		}

		delete(m_unwrapper);
	}

	/*
	 * Set the phase of the background to the phase calculated from the given image
	 */
	template <typename T = double>
	void setBackground(T intensity, int dim_x, int dim_y) {
		// Test whether we have to reinitialize the FFT plan
		initialize(dim_x, dim_y);

		// Initialize the input array
		copyToInput(m_in_FFT, intensity, dim_x, dim_y);
		
		// Calculate the Fourier transform (FFT)
		fftw_execute(m_FFT);

		background_findCenter(dim_x, dim_y);

		getRawPhase();

		int N = dim_x * dim_y;
		memcpy(m_background, m_out_IFFT, , sizeof(fftw_complex) * N);
	}

	/*
	 * Find the center of the background peak
	 */
	void background_findCenter(int dim_x, int dim_y) {

		if (m_dim_background_x != dim_x || m_dim_background_y != dim_y) {
			if (m_background != nullptr) {
				fftw_free(m_background);
			}
			m_background = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * dim_x * dim_y);
		}

		m_dim_background_x = dim_x;
		m_dim_background_y = dim_y;

		// Calculate magnitude of background image to find the indices of the largest element
		std::vector<double> background;
		background.resize(dim_x * dim_y);
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			background[i] = pow(m_out_FFT[i][0], 2) + pow(m_out_FFT[i][1], 2);
		}

		// Find indices of largest element in given range
		int left = round(dim_y * 0.05) * dim_y;
		int right = round(dim_y * 0.45) * dim_y;
		std::vector<double>::iterator itl = std::begin(background);
		std::vector<double>::iterator itr = std::begin(background);
		std::advance(itl, left);
		std::advance(itr, right);
		std::vector<double>::iterator result = std::max_element(itl, itr);
		int ind = std::distance(std::begin(background), result);
		
		m_max_y = floor(ind / dim_x);
		m_max_x = ind - m_max_y * dim_x;

		m_max_x = round(m_max_x - dim_x / 2.0 - 1);
		m_max_y = round(m_max_y - dim_y / 2.0 - 1);
	}

	template <typename T_in = double, typename T_out = double>
	void calculateSpectrum(T_in* intensity, T_out* spectrum, int dim_x, int dim_y) {
		// Test whether we have to reinitialize the FFT plan
		initialize(dim_x, dim_y);

		// Initialize the input array
		copyToInput(m_in_FFT, intensity, dim_x, dim_y);

		// Calculate the FFT
		fftw_execute(m_FFT);

		// Calculate the absolute value
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			(*spectrum)[i] = log10(sqrt(pow(m_out_FFT[i][0], 2) + pow(m_out_FFT[i][1], 2)) / (dim_x * dim_y));
		}

		fftshift(&(*spectrum)[0], dim_x, dim_y);
	}

	template <typename T_in = double, typename T_out = double>
	void calculatePhase(T_in* intensity, T_out* phase, int dim_x, int dim_y) {
		// Test whether we have to reinitialize the FFT plan
		initialize(dim_x, dim_y);
		
		// Initialize the input array
		copyToInput(m_in_FFT, intensity, dim_x, dim_y);
		
		// Calculate the Fourier transform (FFT)
		fftw_execute(m_FFT);

		// If we have no background yet (or the background does not have the correct size),
		// we use the intensity image we just got
		bool updateBackground = m_background == nullptr || (m_dim_background_x != dim_x || m_dim_background_y != dim_y);
		if (updateBackground || m_updateBackground) {
			background_findCenter(dim_x, dim_y);
		}

		// Calculate the phase difference
		getRawPhase();

		int N = dim_x * dim_y;
		if (updateBackground || m_updateBackground) {
			m_updateBackground = false;
			memcpy(m_background, m_out_IFFT, sizeof(fftw_complex) * N);
		}

		// divide by background
		double a, b, c, d, e;
		for (int i{ 0 }; i < N; i++) {
			a = m_out_IFFT[i][0];
			b = m_out_IFFT[i][1];
			c = m_background[i][0];
			d = m_background[i][1];
			e = (pow(c, 2) + pow(d, 2));
			m_out_IFFT[i][0] = (a * c + b * d) / e;
			m_out_IFFT[i][1] = (b * c - a * d) / e;
		}

		// Calculate the phase angle
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			(*phase)[i] = atan2(m_out_IFFT[i][1], m_out_IFFT[i][0]);
		}

		// Downsample the image to speed up unwrapping
		int dim_x_new = dim_x / 2;
		int dim_y_new = dim_y / 2;
		std::vector<float> phase_lowRes;
		phase_lowRes.resize(dim_x_new * dim_y_new);
		m_xsample->resample(&(*phase)[0], &phase_lowRes[0], dim_x, dim_y, dim_x_new, dim_y_new, RESAMPLE_MODE::NEAREST);

		std::vector<float> phaseUnwrapped = phase_lowRes;
		m_unwrapper->unwrap2DWrapped(&phase_lowRes[0], &phaseUnwrapped[0], dim_x_new, dim_y_new, false, false);

		// Subtract median value
		auto newPhase = phaseUnwrapped;
		auto beg = std::begin(newPhase);
		auto end = std::end(newPhase);
		auto median = simplemath::median(beg, end);

		for (int i{ 0 }; i < dim_x_new * dim_y_new; i++) {
			phaseUnwrapped[i] = phaseUnwrapped[i] - median;
		}

		// Upsample the image to match input resolution
		m_xsample->resample(&phaseUnwrapped[0], &(*phase)[0], dim_x_new, dim_y_new, dim_x, dim_y, RESAMPLE_MODE::LINEAR);
	}

	template <typename T = double>
	static void fftshift(T* inputArray, int dim_x, int dim_y) {
		size_t xshift = floor(dim_x / 2.0);
		size_t yshift = floor(dim_y / 2.0);
		circshift(inputArray, dim_x, dim_y, xshift, yshift);
	}

	template <typename T = double>
	static void ifftshift(T* inputArray, int dim_x, int dim_y) {
		size_t xshift = ceil(dim_x / 2.0);
		size_t yshift = ceil(dim_y / 2.0);
		circshift(inputArray, dim_x, dim_y, xshift, yshift);
	}

	// Taken (and slightly adjusted to be circshift, not fftshift) from this SO answer
	// https://stackoverflow.com/questions/5915125/fftshift-ifftshift-c-c-source-code/36186981#36186981
	template <typename T = double>
	static void circshift(T* inputArray, int dim_x, int dim_y, int xshift, int yshift) {
		if ((dim_x*dim_y) % 2 == 0 && dim_x == 2*xshift && dim_y == 2*yshift) {
			// in and output array are the same and we shift by half the size,
			// values are exchanged using swap
			for (size_t x{ 0 }; x < dim_x; x++) {
				size_t outX = (x + xshift) % dim_x;
				for (size_t y{ 0 }; y < yshift; y++) {
					size_t outY = (y + yshift) % dim_y;
					// row-major order
					std::swap(inputArray[outX + dim_x * outY], inputArray[x + dim_x * y]);
				}
			}
		} else {
			// temp output array
			int N = dim_x * dim_y;
			T* out = (T*)malloc(sizeof(T) * N);
			for (size_t x{ 0 }; x < dim_x; x++) {
				size_t outX = (x + xshift) % dim_x;
				for (size_t y{ 0 }; y < dim_y; y++) {
					size_t outY = (y + yshift) % dim_y;
					// row-major order
					memcpy(&out[outX + dim_x * outY], &inputArray[x + dim_x * y], sizeof(T));
				}
			}
			// copy out back to data
			memcpy(inputArray, out, sizeof(T) * N);
			free(out);
		}
	}

};

#endif // PHASE_H