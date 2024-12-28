#include "timelineview.h"
#include "timelineruler.h"
#include "timelineclip.h"
#include <QWheelEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>

TimelineView::TimelineView(QWidget* parent)
    : QGraphicsView(parent)
    , zoomLevel(1.0)
    , viewportStartTime(0.0)
    , viewportDuration(60.0)  // Initial viewport shows 60 seconds
{
    scene = new QGraphicsScene(this);
    setScene(scene);
    
    // Setup the view
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    
    // Create and add the ruler
    ruler = new TimelineRuler();
    scene->addItem(ruler);
    
    // Accept drops for drag and drop functionality
    setAcceptDrops(true);
    
    // Set initial scene rect
    updateTimelineRange();
}

TimelineView::~TimelineView() {
    delete scene;
}

void TimelineView::addTrack(TrackType type) {
    auto track = std::make_shared<TimelineTrack>(type, tracks.size());
    tracks.push_back(track);
    scene->addItem(track.get());
    updateTrackPositions();
    updateTimelineRange();
}

void TimelineView::removeTrack(int trackIndex) {
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        scene->removeItem(tracks[trackIndex].get());
        tracks.erase(tracks.begin() + trackIndex);
        updateTrackPositions();
        updateTimelineRange();
    }
}

void TimelineView::addClip(const QString& filePath, int trackIndex, qreal startTime, qreal duration) {
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        auto clip = std::make_shared<TimelineClip>(filePath, startTime, duration);
        tracks[trackIndex]->addClip(clip);
    }
}

void TimelineView::updateTimelineRange() {
    qreal totalHeight = tracks.size() * TimelineTrack::TRACK_HEIGHT + TimelineRuler::RULER_HEIGHT;
    scene->setSceneRect(0, 0, TimelineTrack::TRACK_WIDTH * zoomLevel, totalHeight);
    ruler->setZoomLevel(zoomLevel);
    ruler->setViewportRange(viewportStartTime, viewportDuration);
}

void TimelineView::zoomIn() {
    setZoomLevel(zoomLevel * ZOOM_STEP);
}

void TimelineView::zoomOut() {
    setZoomLevel(zoomLevel / ZOOM_STEP);
}

void TimelineView::setZoomLevel(qreal level) {
    zoomLevel = qBound(MIN_ZOOM, level, MAX_ZOOM);
    updateTimelineRange();
}

void TimelineView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom with Ctrl + Mouse Wheel
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        // Normal scrolling
        QGraphicsView::wheelEvent(event);
    }
}

void TimelineView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TimelineView::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void TimelineView::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QPointF pos = mapToScene(event->pos());
        int trackIndex = getTrackIndexAtPosition(pos);
        qreal dropTime = getTimeAtPosition(pos.x());
        
        if (trackIndex >= 0 && trackIndex < tracks.size()) {
            for (const QUrl& url : mimeData->urls()) {
                QString filePath = url.toLocalFile();
                // Add clip with default duration of 5 seconds
                addClip(filePath, trackIndex, dropTime, 5.0);
            }
        }
    }
    event->acceptProposedAction();
}

void TimelineView::updateTrackPositions() {
    qreal y = TimelineRuler::RULER_HEIGHT;
    for (const auto& track : tracks) {
        track->setPos(0, y);
        y += TimelineTrack::TRACK_HEIGHT;
    }
}

int TimelineView::getTrackIndexAtPosition(const QPointF& scenePos) const {
    if (scenePos.y() < TimelineRuler::RULER_HEIGHT) {
        return -1;
    }
    return static_cast<int>((scenePos.y() - TimelineRuler::RULER_HEIGHT) / TimelineTrack::TRACK_HEIGHT);
}

qreal TimelineView::getTimeAtPosition(qreal x) const {
    return x / (TimelineClip::PIXELS_PER_SECOND * zoomLevel);
}
