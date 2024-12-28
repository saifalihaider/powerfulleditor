#include "audiomanager.h"
#include <QProcess>
#include <QDebug>

AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
{
}

std::shared_ptr<AudioTrack> AudioManager::addTrack() {
    auto track = std::make_shared<AudioTrack>();
    connect(track.get(), &AudioTrack::trackChanged, this, &AudioManager::tracksChanged);
    tracks.append(track);
    emit tracksChanged();
    return track;
}

void AudioManager::removeTrack(int index) {
    if (index >= 0 && index < tracks.size()) {
        tracks.removeAt(index);
        emit tracksChanged();
    }
}

void AudioManager::addClip(int trackIndex, const QString& filePath, qreal startTime, qreal duration) {
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        auto clip = std::make_shared<AudioClip>(filePath, startTime, duration);
        tracks[trackIndex]->addClip(clip);
    }
}

void AudioManager::removeClip(int trackIndex, int clipIndex) {
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        auto& clips = tracks[trackIndex]->getClips();
        if (clipIndex >= 0 && clipIndex < clips.size()) {
            tracks[trackIndex]->removeClip(clips[clipIndex]);
        }
    }
}

QString AudioManager::generateMixingCommand(const QString& outputFile) const {
    QString command = "ffmpeg";
    
    // Add input files
    for (const auto& track : tracks) {
        for (const auto& clip : track->getClips()) {
            command += QString(" -i \"%1\"").arg(clip->getFilePath());
        }
    }
    
    // Add filter complex
    command += " -filter_complex \"" + generateFilterComplex() + "\"";
    
    // Add output options
    command += QString(" -y \"%1\"").arg(outputFile);
    
    return command;
}

QString AudioManager::generateExportCommand(const QString& videoFile, const QString& outputFile) const {
    QString command = QString("ffmpeg -i \"%1\"").arg(videoFile);
    
    // Add audio inputs
    for (const auto& track : tracks) {
        for (const auto& clip : track->getClips()) {
            command += QString(" -i \"%1\"").arg(clip->getFilePath());
        }
    }
    
    // Add filter complex
    command += " -filter_complex \"" + generateFilterComplex() + "\"";
    
    // Copy video stream and use mixed audio
    command += " -map 0:v:0 -map [mixout] -c:v copy -c:a aac";
    
    // Output file
    command += QString(" -y \"%1\"").arg(outputFile);
    
    return command;
}

bool AudioManager::generatePreview(const QString& outputFile, qreal startTime, qreal duration) {
    QString command = generateMixingCommand(outputFile);
    command.insert(6, QString(" -ss %1 -t %2").arg(startTime).arg(duration));
    return runFFmpegCommand(command);
}

bool AudioManager::exportAudio(const QString& outputFile) {
    emit processingStarted();
    bool success = runFFmpegCommand(generateMixingCommand(outputFile));
    emit processingFinished(success);
    return success;
}

bool AudioManager::combineWithVideo(const QString& videoFile, const QString& outputFile) {
    emit processingStarted();
    bool success = runFFmpegCommand(generateExportCommand(videoFile, outputFile));
    emit processingFinished(success);
    return success;
}

bool AudioManager::runFFmpegCommand(const QString& command) {
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

QString AudioManager::generateFilterComplex() const {
    QStringList filterParts;
    QStringList mixInputs;
    int inputIndex = 0;
    
    // Process each track
    for (const auto& track : tracks) {
        if (track->isMuted()) continue;
        
        QStringList trackInputs;
        for (const auto& clip : track->getClips()) {
            QString clipFilter = QString("[%1]%2[clip%1]")
                .arg(inputIndex)
                .arg(clip->getFFmpegFilter());
            filterParts.append(clipFilter);
            trackInputs.append(QString("[clip%1]").arg(inputIndex));
            inputIndex++;
        }
        
        // Concatenate clips in the track if there are multiple
        if (trackInputs.size() > 1) {
            QString trackMix = QString("%1concat=n=%2:v=0:a=1[track%3]")
                .arg(trackInputs.join(""))
                .arg(trackInputs.size())
                .arg(inputIndex);
            filterParts.append(trackMix);
            mixInputs.append(QString("[track%1]").arg(inputIndex));
        } else if (trackInputs.size() == 1) {
            mixInputs.append(trackInputs.first());
        }
    }
    
    // Mix all tracks together
    if (mixInputs.size() > 0) {
        QString finalMix = QString("%1amix=inputs=%2[mixout]")
            .arg(mixInputs.join(""))
            .arg(mixInputs.size());
        filterParts.append(finalMix);
    }
    
    return filterParts.join(";");
}
