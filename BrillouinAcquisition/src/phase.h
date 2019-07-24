#ifndef PHASE_H
#define PHASE_H

#include <vector>
#include <iterator>
#include <complex>
#include <utility>

#include <math.h>

#include "../external/fftw/fftw3.h"

class phase {

private:

	double *m_backgroundPhase = nullptr;
	fftw_complex *m_in;
	fftw_complex *m_out;
	fftw_plan m_plan;
	int m_dim_x{ 0 }, m_dim_y{ 0 };
	bool m_initialized{ false };

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

			int N = m_dim_x * m_dim_y;

			if (m_initialized) {
				fftw_destroy_plan(m_plan);
				fftw_free(m_in);
				fftw_free(m_out);
			}

			m_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
			m_plan = fftw_plan_dft_2d(m_dim_x, m_dim_y, m_in, m_out, FFTW_FORWARD, FFTW_ESTIMATE);

			m_initialized = true;
		}
	}

	template <typename T = double>
	void getRawPhase(T* intensity, T* phase, int dim_x, int dim_y) {

		// Calculate the frequency information (FFT)
		fftw_execute(m_plan);

		// Select sample information

		// Calculate the phase (Inverse FFT)
	}

public:

	phase() {}

	~phase() {
		fftw_destroy_plan(m_plan);
		fftw_free(m_in);
		fftw_free(m_out);
	}

	/*
	 * Set the phase of the background to the phase calculated from the given image
	 */
	template <typename T = double>
	void setBackground(T intensity, int dim_x, int dim_y) {

		//getRawPhase(intensity, dim_x, dim_y);
	}

	template <typename T_in = double, typename T_out = double>
	void calculateSpectrum(T_in* intensity, T_out* spectrum, int dim_x, int dim_y) {
		// Test whether we have to reinitialize the FFT plan
		initialize(dim_x, dim_y);

		// Initialize the input array
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			m_in[i][0] = intensity[i];
			m_in[i][1] = 0.0;
		}

		// Calculate the FFT
		fftw_execute(m_plan);

		// Calculate the absolute value
		for (int i{ 0 }; i < dim_x * dim_y; i++) {
			spectrum[i] = log10(sqrt(pow(m_out[i][0], 2) + pow(m_out[i][1], 2)) / (dim_x * dim_y));
		}

		fftshift(spectrum, dim_y, dim_x);
	}

	template <typename T = double>
	void calculatePhase(T* intensity, T* phase, int dim_x, int dim_y) {

		//// If we have no background yet (or the background does not have the correct size),
		//// we use the intensity image we just got
		//if (m_backgroundPhase == nullptr || (m_dim_x != dim_x || m_dim_y != dim_y)) {
		//	setBackground(intensity, dim_x, dim_y);
		//}

		//// Calculate the phase difference
		//getRawPhase(intensity, phase, dim_x, dim_y);

		//for (int i{ 0 }; i < dim_x * dim_y; i++) {
		//	phase[i] = phase[i] - m_backgroundPhase[i];
		//}
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
			std::vector<double> out;
			out.resize(dim_x * dim_y);
			for (size_t x{ 0 }; x < dim_x; x++) {
				size_t outX = (x + xshift) % dim_x;
				for (size_t y{ 0 }; y < dim_y; y++) {
					size_t outY = (y + yshift) % dim_y;
					// row-major order
					out[outX + dim_x * outY] = inputArray[x + dim_x * y];
				}
			}
			// copy out back to data
			copy(out.begin(), out.end(), &inputArray[0]);
		}
	}

};

#endif // PHASE_H