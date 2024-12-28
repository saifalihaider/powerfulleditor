#include "audiotrack.h"

AudioClip::AudioClip(const QString& filePath, qreal startTime, qreal duration)
    : filePath(filePath)
    , startTime(startTime)
    , duration(duration)
    , muted(false)
    , volume(1.0)
{
}

QString AudioClip::getFFmpegFilter() const {
    if (muted) {
        return "volume=0";
    }
    return QString("volume=%1").arg(volume);
}

AudioTrack::AudioTrack(QObject* parent)
    : QObject(parent)
    , muted(false)
    , solo(false)
    , volume(1.0)
{
}

void AudioTrack::addClip(std::shared_ptr<AudioClip> clip) {
    clips.append(clip);
    emit trackChanged();
}

void AudioTrack::removeClip(std::shared_ptr<AudioClip> clip) {
    clips.removeOne(clip);
    emit trackChanged();
}

void AudioTrack::setMuted(bool value) {
    if (muted != value) {
        muted = value;
        emit muteChanged(muted);
        emit trackChanged();
    }
}

void AudioTrack::setSolo(bool value) {
    if (solo != value) {
        solo = value;
        emit soloChanged(solo);
        emit trackChanged();
    }
}

void AudioTrack::setVolume(double value) {
    if (volume != value) {
        volume = value;
        emit volumeChanged(volume);
        emit trackChanged();
    }
}

void AudioTrack::addEffect(std::unique_ptr<AudioEffect> effect) {
    effects.append(std::move(effect));
    emit trackChanged();
}

void AudioTrack::removeEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        effects.removeAt(index);
        emit trackChanged();
    }
}

QString AudioTrack::getFFmpegFilter() const {
    if (muted) {
        return "volume=0";
    }
    
    QStringList filters;
    
    // Add track volume
    filters.append(QString("volume=%1").arg(volume));
    
    // Add effects
    for (const auto& effect : effects) {
        QString filter = effect->getFFmpegFilter();
        if (!filter.isEmpty()) {
            filters.append(filter);
        }
    }
    
    return filters.join(",");
}
