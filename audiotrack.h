#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <memory>
#include "audioeffect.h"

class AudioClip {
public:
    AudioClip(const QString& filePath, qreal startTime, qreal duration);
    
    QString getFilePath() const { return filePath; }
    qreal getStartTime() const { return startTime; }
    qreal getDuration() const { return duration; }
    bool isMuted() const { return muted; }
    double getVolume() const { return volume; }
    
    void setStartTime(qreal time) { startTime = time; }
    void setDuration(qreal dur) { duration = dur; }
    void setMuted(bool value) { muted = value; }
    void setVolume(double value) { volume = value; }
    
    QString getFFmpegFilter() const;

private:
    QString filePath;
    qreal startTime;
    qreal duration;
    bool muted;
    double volume;
};

class AudioTrack : public QObject {
    Q_OBJECT

public:
    explicit AudioTrack(QObject* parent = nullptr);
    
    // Clip management
    void addClip(std::shared_ptr<AudioClip> clip);
    void removeClip(std::shared_ptr<AudioClip> clip);
    const QList<std::shared_ptr<AudioClip>>& getClips() const { return clips; }
    
    // Track properties
    bool isMuted() const { return muted; }
    bool isSolo() const { return solo; }
    double getVolume() const { return volume; }
    
    void setMuted(bool value);
    void setSolo(bool value);
    void setVolume(double value);
    
    // Effects
    void addEffect(std::unique_ptr<AudioEffect> effect);
    void removeEffect(int index);
    const QList<std::unique_ptr<AudioEffect>>& getEffects() const { return effects; }
    
    // Generate FFmpeg filter string for this track
    QString getFFmpegFilter() const;

signals:
    void trackChanged();
    void muteChanged(bool muted);
    void soloChanged(bool solo);
    void volumeChanged(double volume);

private:
    QList<std::shared_ptr<AudioClip>> clips;
    QList<std::unique_ptr<AudioEffect>> effects;
    bool muted;
    bool solo;
    double volume;
};
