#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtCore>
#include <gsl/gsl>

#include "previewBuffer.h"
#include "external/qcustomplot/qcustomplot.h"
#include "phase.h"

enum class CustomGradientPreset {
	gpViridis,
	gpGrayscale,
	gpRed,
	gpGreen,
	gpBlue,
	gpInferno
};

typedef enum class displayMode {
	INTENSITY,
	SPECTRUM,
	PHASE
} DISPLAY_MODE;

struct PLOT_SETTINGS {
	QCustomPlot* plotHandle{ nullptr };
	QCPColorMap* colorMap{ nullptr };
	QCPRange cLim = { 100, 300 };
	QSpinBox* lowerBox{ nullptr };
	QSpinBox* upperBox{ nullptr };
	std::function<void(QCPRange)> dataRangeCallback{ nullptr };
	bool autoscale{ false };
	CustomGradientPreset gradient = CustomGradientPreset::gpViridis;
	DISPLAY_MODE mode{ DISPLAY_MODE::INTENSITY };
};

class converter : public QObject {
	Q_OBJECT

public:
	converter();
	~converter();

public slots:
	void init();

	void convert(PreviewBuffer<std::byte>* previewBuffer, PLOT_SETTINGS* plotSettings);

	void updateBackground();

private:
	phase* m_phase{ nullptr };

	template <typename T = double>
	void conv(PreviewBuffer<std::byte>* previewBuffer, PLOT_SETTINGS* plotSettings, T* unpackedBuffer);

signals:
	void s_converted(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<unsigned char> unpackedBuffer);
	void s_converted(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<unsigned short> unpackedBuffer);
	void s_converted(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<double> unpackedBuffer);
	void s_converted(PLOT_SETTINGS* plotSettings, long long dim_x, long long dim_y, std::vector<float> unpackedBuffer);

};

#endif // PLOTTER_H