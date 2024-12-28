#include "timelinetrack.h"
#include "timelineclip.h"
#include <QPen>
#include <QBrush>

TimelineTrack::TimelineTrack(TrackType type, int trackIndex, QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , type(type)
    , trackIndex(trackIndex)
    , muted(false)
    , solo(false)
{
    setRect(0, trackIndex * TRACK_HEIGHT, TRACK_WIDTH, TRACK_HEIGHT);
    
    // Set track appearance based on type
    QPen pen(Qt::black);
    QBrush brush;
    switch (type) {
        case TrackType::Video:
            brush = QBrush(QColor(100, 149, 237, 127));  // Cornflower blue
            break;
        case TrackType::Audio:
            brush = QBrush(QColor(144, 238, 144, 127));  // Light green
            break;
        case TrackType::Text:
            brush = QBrush(QColor(255, 182, 193, 127));  // Light pink
            break;
    }
    
    setPen(pen);
    setBrush(brush);
    setAcceptDrops(true);
}

void TimelineTrack::setMuted(bool value) {
    muted = value;
    // Update visual feedback for muted state
    setBrush(muted ? brush().color().darker(150) : brush().color());
}

void TimelineTrack::setSolo(bool value) {
    solo = value;
    // Update visual feedback for solo state
    setPen(solo ? QPen(Qt::yellow, 2) : QPen(Qt::black, 1));
}

void TimelineTrack::addClip(std::shared_ptr<TimelineClip> clip) {
    clips.push_back(clip);
    clip->setParentItem(this);
}

void TimelineTrack::removeClip(std::shared_ptr<TimelineClip> clip) {
    auto it = std::find(clips.begin(), clips.end(), clip);
    if (it != clips.end()) {
        clips.erase(it);
        clip->setParentItem(nullptr);
    }
}
