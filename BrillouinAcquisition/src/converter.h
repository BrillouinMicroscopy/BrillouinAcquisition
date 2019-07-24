#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtCore>
#include <gsl/gsl>

#include "previewBuffer.h"
#include "external/qcustomplot/qcustomplot.h"
#include "phase.h"

enum CustomGradientPreset {
	gpParula,
	gpGrayscale,
	gpRed,
	gpGreen,
	gpBlue
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
	QSpinBox* lowerBox;
	QSpinBox* upperBox;
	std::function<void(QCPRange)> dataRangeCallback{ nullptr };
	bool autoscale{ false };
	CustomGradientPreset gradient = CustomGradientPreset::gpParula;
	DISPLAY_MODE mode{ DISPLAY_MODE::SPECTRUM };
};

class converter : public QObject {
	Q_OBJECT

private:
	phase* m_phase = nullptr;

	template <typename T = double>
	void conv(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, T* unpackedBuffer);

public:
	converter();
	~converter();

public slots:
	void init();

	void convert(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, unsigned char* unpackedBuffer);
	void convert(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS* plotSettings, unsigned short* unpackedBuffer);

signals:
	void s_converted(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<unsigned char> unpackedBuffer);
	void s_converted(PreviewBuffer<unsigned char>* previewBuffer, PLOT_SETTINGS * plotSettings, std::vector<unsigned short> unpackedBuffer);

};

#endif // PLOTTER_H