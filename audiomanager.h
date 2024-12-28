#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <memory>
#include "audiotrack.h"

class AudioManager : public QObject {
    Q_OBJECT

public:
    explicit AudioManager(QObject* parent = nullptr);
    
    // Track management
    std::shared_ptr<AudioTrack> addTrack();
    void removeTrack(int index);
    const QList<std::shared_ptr<AudioTrack>>& getTracks() const { return tracks; }
    
    // Clip management
    void addClip(int trackIndex, const QString& filePath, qreal startTime, qreal duration);
    void removeClip(int trackIndex, int clipIndex);
    
    // Generate FFmpeg commands for audio processing
    QString generateMixingCommand(const QString& outputFile) const;
    QString generateExportCommand(const QString& videoFile, const QString& outputFile) const;
    
    // Preview functionality
    bool generatePreview(const QString& outputFile, qreal startTime, qreal duration);
    
    // Export functionality
    bool exportAudio(const QString& outputFile);
    bool combineWithVideo(const QString& videoFile, const QString& outputFile);

signals:
    void tracksChanged();
    void processingStarted();
    void processingFinished(bool success);
    void progressUpdated(int percent);

private:
    QList<std::shared_ptr<AudioTrack>> tracks;
    
    // Helper functions
    bool runFFmpegCommand(const QString& command);
    QString generateFilterComplex() const;
};
