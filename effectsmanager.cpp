#include "effectsmanager.h"
#include <QProcess>
#include <QDebug>

EffectsManager::EffectsManager(QObject* parent)
    : QObject(parent)
{
}

void EffectsManager::addEffect(std::unique_ptr<VideoEffect> effect) {
    effects.append(std::move(effect));
    emit effectsChanged();
}

void EffectsManager::removeEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        effects.removeAt(index);
        emit effectsChanged();
    }
}

void EffectsManager::clearEffects() {
    effects.clear();
    emit effectsChanged();
}

QString EffectsManager::generateFilterString() const {
    QStringList filters;
    for (const auto& effect : effects) {
        QString filter = effect->getFFmpegFilter();
        if (!filter.isEmpty()) {
            filters.append(filter);
        }
    }
    return filters.join(",");
}

bool EffectsManager::applyEffects(const QString& inputFile, const QString& outputFile) {
    emit processingStarted();
    
    QString filterString = generateFilterString();
    if (filterString.isEmpty()) {
        // No effects to apply, just copy the file
        QString command = QString("ffmpeg -i \"%1\" -c copy \"%2\"")
            .arg(inputFile)
            .arg(outputFile);
        bool success = runFFmpegCommand(command);
        emit processingFinished(success);
        return success;
    }
    
    QString command = QString("ffmpeg -i \"%1\" -vf \"%2\" -c:a copy \"%3\"")
        .arg(inputFile)
        .arg(filterString)
        .arg(outputFile);
    
    bool success = runFFmpegCommand(command);
    emit processingFinished(success);
    return success;
}

bool EffectsManager::generatePreviewFrame(const QString& inputFile, const QString& outputFile, double timestamp) {
    QString filterString = generateFilterString();
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

bool EffectsManager::runFFmpegCommand(const QString& command) {
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
        // This is a simplified progress estimation
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
