#include "textmanager.h"
#include <QProcess>
#include <QDebug>

TextManager::TextManager(QObject* parent)
    : QObject(parent)
{
}

void TextManager::addTextEffect(std::unique_ptr<TextEffect> effect) {
    textEffects.append(std::move(effect));
    emit textEffectsChanged();
}

void TextManager::removeTextEffect(int index) {
    if (index >= 0 && index < textEffects.size()) {
        textEffects.removeAt(index);
        emit textEffectsChanged();
    }
}

void TextManager::updateTextEffect(int index, const TextEffect& effect) {
    if (index >= 0 && index < textEffects.size()) {
        *textEffects[index] = effect;
        emit textEffectsChanged();
    }
}

QString TextManager::generateFilterString(int videoWidth, int videoHeight) const {
    QStringList filters;
    
    for (const auto& effect : textEffects) {
        QString filter = effect->getFFmpegFilter(videoWidth, videoHeight);
        if (!filter.isEmpty()) {
            filters.append(filter);
        }
    }
    
    return filters.join(",");
}

bool TextManager::applyTextEffects(const QString& inputFile, const QString& outputFile) {
    emit processingStarted();
    
    // Get video dimensions
    QString probeCommand = QString("ffprobe -v error -select_streams v:0 "
                                 "-show_entries stream=width,height -of csv=p=0 \"%1\"")
        .arg(inputFile);
    
    QProcess probeProcess;
    probeProcess.start(probeCommand);
    probeProcess.waitForFinished();
    
    QString dimensions = QString::fromUtf8(probeProcess.readAllStandardOutput()).trimmed();
    QStringList dims = dimensions.split(",");
    if (dims.size() != 2) {
        qDebug() << "Failed to get video dimensions";
        emit processingFinished(false);
        return false;
    }
    
    int width = dims[0].toInt();
    int height = dims[1].toInt();
    
    // Generate filter string
    QString filterString = generateFilterString(width, height);
    if (filterString.isEmpty()) {
        // No text effects to apply, just copy the file
        QString command = QString("ffmpeg -i \"%1\" -c copy \"%2\"")
            .arg(inputFile)
            .arg(outputFile);
        bool success = runFFmpegCommand(command);
        emit processingFinished(success);
        return success;
    }
    
    // Apply text effects
    QString command = QString("ffmpeg -i \"%1\" -vf \"%2\" -c:a copy \"%3\"")
        .arg(inputFile)
        .arg(filterString)
        .arg(outputFile);
    
    bool success = runFFmpegCommand(command);
    emit processingFinished(success);
    return success;
}

bool TextManager::generatePreviewFrame(const QString& inputFile, const QString& outputFile,
                                     double timestamp, int width, int height) {
    QString filterString = generateFilterString(width, height);
    QString command;
    
    if (filterString.isEmpty()) {
        command = QString("ffmpeg -ss %1 -i \"%2\" -vframes 1 \"%3\"")
            .arg(timestamp)
            .arg(inputFile)
            .arg(outputFile);
    } else {
        command = QString("ffmpeg -ss %1 -i \"%2\" -vf \"%3\" -vframes 1 \"%4\"")
            .arg(timestamp)
            .arg(inputFile)
            .arg(filterString)
            .arg(outputFile);
    }
    
    return runFFmpegCommand(command);
}

bool TextManager::runFFmpegCommand(const QString& command) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    qDebug() << "Running FFmpeg command:" << command;
    
    process.start(command);
    if (!process.waitForStarted()) {
        qDebug() << "Failed to start FFmpeg process";
        return false;
    }
    
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead();
        QString output = process.readAll();
        
        // Parse FFmpeg output to estimate progress
        if (output.contains("time=")) {
            int percent = 50;  // Simplified progress reporting
            emit progressUpdated(percent);
        }
    }
    
    bool success = (process.exitCode() == 0);
    if (!success) {
        qDebug() << "FFmpeg process failed:" << process.readAll();
    }
    
    return success;
}
