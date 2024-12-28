#pragma once

#include <QGraphicsRectItem>
#include <QString>
#include <vector>
#include <memory>

enum class TrackType {
    Video,
    Audio,
    Text
};

class TimelineClip;

class TimelineTrack : public QGraphicsRectItem {
public:
    explicit TimelineTrack(TrackType type, int trackIndex, QGraphicsItem* parent = nullptr);

    TrackType getType() const { return type; }
    int getTrackIndex() const { return trackIndex; }
    bool isMuted() const { return muted; }
    bool isSolo() const { return solo; }

    void setMuted(bool value);
    void setSolo(bool value);
    void addClip(std::shared_ptr<TimelineClip> clip);
    void removeClip(std::shared_ptr<TimelineClip> clip);

    static constexpr qreal TRACK_HEIGHT = 50.0;
    static constexpr qreal TRACK_WIDTH = 2000.0;

private:
    TrackType type;
    int trackIndex;
    bool muted;
    bool solo;
    std::vector<std::shared_ptr<TimelineClip>> clips;
};
