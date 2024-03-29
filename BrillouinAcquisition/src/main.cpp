#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include <QtWidgets/QApplication>
#include <QFile>
#include <QDir>
#include <QScopedPointer>
#include <QTextStream>
#include <QDateTime>
#include <QLoggingCategory>

#include "src/helper/logger.h"

// Smart pointer for log file
QScopedPointer<QFile> m_logFile;
void loggingHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

int main(int argc, char *argv[]) {

	#ifdef Q_OS_WIN
		SetProcessDPIAware(); // call before the main event loop
	#endif // Q_OS_WIN

	#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	#else
		qputenv("QT_DEVICE_PIXEL_RATIO", QByteArray("1"));
	#endif // QT_VERSION

	QApplication a(argc, argv);
	BrillouinAcquisition w;

	w.setStyleSheet("QPushButton.active  {background-color: rgb(0, 59, 206); color: white}");

	// Set logging file path
	m_logFile.reset(new QFile("log.log"));
	// Open the log file
	m_logFile.data()->open(QFile::Append | QFile::Text);
	// Set handler message handler
	qInstallMessageHandler(loggingHandler);

	w.show();
	qInfo(logInfo()) << "BrillouinAcquisition started.";
	return a.exec();
}

void loggingHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	// Open stream for logging
	QTextStream out(m_logFile.data());
	// Log datetime
	out << QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODateWithMs) << ": ";
	// Check log level
	switch (type) {
		case QtInfoMsg:     out << "I/"; break;
		case QtDebugMsg:    out << "D/"; break;
		case QtWarningMsg:  out << "W/"; break;
		case QtCriticalMsg: out << "E/"; break;
		case QtFatalMsg:    out << "F/"; break;
	}
	// Only log file, function and line if it is not an info
	if (type < 4) {
		out << context.file << "[" << context.function << "](" << context.line << ")";
	}
	// Log the message
	out << ": " << msg << endl;
	// Clear the stream
	out.flush();
}