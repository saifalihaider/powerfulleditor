#include "videoexporter.h"
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>

VideoExporter::VideoExporter(QObject* parent)
    : QObject(parent)
    , exportProcess(std::make_unique<QProcess>())
    , progress(0.0)
{
    // Set up process
    exportProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    // Connect signals
    connect(exportProcess.get(), &QProcess::readyRead,
            this, &VideoExporter::handleProcessOutput);
    connect(exportProcess.get(), &QProcess::errorOccurred,
            this, &VideoExporter::handleProcessError);
    connect(exportProcess.get(), 
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &VideoExporter::handleProcessFinished);
    
    // Set up progress timer
    progressTimer.setInterval(500);  // Update progress every 500ms
    connect(&progressTimer, &QTimer::timeout,
            this, &VideoExporter::updateProgress);
}

VideoExporter::~VideoExporter() {
    if (isExporting()) {
        cancelExport();
    }
}

bool VideoExporter::startExport(const QString& inputFile, const QString& outputFile) {
    if (isExporting()) {
        reportError("Export already in progress");
        return false;
    }
    
    // Check input file
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists() || !inputInfo.isReadable()) {
        reportError("Input file does not exist or is not readable");
        return false;
    }
    
    // Check output directory
    QFileInfo outputInfo(outputFile);
    QDir outputDir = outputInfo.dir();
    if (!outputDir.exists() && !outputDir.mkpath(".")) {
        reportError("Cannot create output directory");
        return false;
    }
    
    // Build FFmpeg command
    QStringList arguments = buildFFmpegCommand(inputFile, outputFile);
    
    // Start export process
    exportProcess->start("ffmpeg", arguments);
    if (!exportProcess->waitForStarted()) {
        reportError("Failed to start FFmpeg process");
        return false;
    }
    
    // Reset progress and start timer
    progress = 0.0;
    progressTimer.start();
    
    emit exportStarted();
    return true;
}

void VideoExporter::cancelExport() {
    if (isExporting()) {
        exportProcess->kill();
        progressTimer.stop();
        emit exportCancelled();
    }
}

qint64 VideoExporter::estimateFileSize() const {
    // Estimate video size
    qint64 videoBitsPerSecond = exportSettings.getVideoBitrate() * 1000;
    qint64 audioBitsPerSecond = exportSettings.isAudioEnabled() ? 
                               exportSettings.getAudioBitrate() * 1000 : 0;
    
    // Total bits per second
    qint64 totalBitsPerSecond = videoBitsPerSecond + audioBitsPerSecond;
    
    // Assume 1 minute of video for estimation
    qint64 estimatedBytes = (totalBitsPerSecond / 8) * 60;
    
    return estimatedBytes;
}

bool VideoExporter::generatePreview(const QString& inputFile, 
                                  const QString& outputFile,
                                  double timestamp) {
    QStringList arguments;
    
    // Seek to timestamp
    arguments << "-ss" << QString::number(timestamp);
    
    // Input file
    arguments << "-i" << inputFile;
    
    // Extract one frame
    arguments << "-vframes" << "1";
    
    // Apply export settings
    arguments << "-s" << QString("%1x%2")
        .arg(exportSettings.getResolution().width())
        .arg(exportSettings.getResolution().height());
    
    // Output file
    arguments << outputFile;
    
    // Run FFmpeg
    QProcess process;
    process.start("ffmpeg", arguments);
    
    if (!process.waitForFinished()) {
        reportError("Failed to generate preview frame");
        return false;
    }
    
    return process.exitCode() == 0;
}

QStringList VideoExporter::buildFFmpegCommand(const QString& inputFile,
                                            const QString& outputFile) const {
    QStringList arguments;
    
    // Input file
    arguments << "-i" << inputFile;
    
    // Add export settings parameters
    arguments << exportSettings.getFFmpegParameters();
    
    // Progress reporting
    arguments << "-progress" << "-" << "-nostats";
    
    // Output file (overwrite if exists)
    arguments << "-y" << outputFile;
    
    return arguments;
}

void VideoExporter::handleProcessOutput() {
    QString output = QString::fromUtf8(exportProcess->readAll());
    
    // Parse progress information
    static QRegularExpression timeRegex("time=([\\d:.]+)");
    QRegularExpressionMatch match = timeRegex.match(output);
    
    if (match.hasMatch()) {
        QString timeStr = match.captured(1);
        double currentTime = parseTime(timeStr);
        double totalDuration = parseDuration(output);
        
        if (totalDuration > 0) {
            progress = (currentTime / totalDuration) * 100.0;
            emit exportProgress(progress);
        }
    }
}

void VideoExporter::handleProcessError(QProcess::ProcessError error) {
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start FFmpeg process";
            break;
        case QProcess::Crashed:
            errorMessage = "FFmpeg process crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "FFmpeg process timed out";
            break;
        case QProcess::WriteError:
            errorMessage = "Failed to write to FFmpeg process";
            break;
        case QProcess::ReadError:
            errorMessage = "Failed to read from FFmpeg process";
            break;
        default:
            errorMessage = "Unknown FFmpeg process error";
    }
    
    reportError(errorMessage);
}

void VideoExporter::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    progressTimer.stop();
    
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        progress = 100.0;
        emit exportProgress(progress);
        emit exportFinished(true);
    } else {
        QString error = QString::fromUtf8(exportProcess->readAllStandardError());
        reportError("Export failed: " + error);
        emit exportFinished(false);
    }
}

void VideoExporter::updateProgress() {
    if (isExporting()) {
        emit exportProgress(progress);
    }
}

double VideoExporter::parseDuration(const QString& output) const {
    QRegularExpression durationRegex("Duration: ([\\d:.]+)");
    QRegularExpressionMatch match = durationRegex.match(output);
    
    if (match.hasMatch()) {
        return parseTime(match.captured(1));
    }
    
    return 0.0;
}

double VideoExporter::parseTime(const QString& timeStr) const {
    QStringList parts = timeStr.split(":");
    if (parts.size() == 3) {
        double hours = parts[0].toDouble();
        double minutes = parts[1].toDouble();
        double seconds = parts[2].toDouble();
        
        return hours * 3600 + minutes * 60 + seconds;
    }
    
    return 0.0;
}

void VideoExporter::reportError(const QString& error) {
    lastError = error;
    qDebug() << "Export error:" << error;
    emit exportError(error);
}
