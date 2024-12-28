#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <vector>
#include <memory>
#include "timelinetrack.h"

class TimelineRuler;

class TimelineView : public QGraphicsView {
    Q_OBJECT

public:
    explicit TimelineView(QWidget* parent = nullptr);
    ~TimelineView();

    void addTrack(TrackType type);
    void removeTrack(int trackIndex);
    void addClip(const QString& filePath, int trackIndex, qreal startTime, qreal duration);
    void updateTimelineRange();

public slots:
    void zoomIn();
    void zoomOut();
    void setZoomLevel(qreal level);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QGraphicsScene* scene;
    TimelineRuler* ruler;
    std::vector<std::shared_ptr<TimelineTrack>> tracks;
    qreal zoomLevel;
    qreal viewportStartTime;
    qreal viewportDuration;

    static constexpr qreal MIN_ZOOM = 0.1;
    static constexpr qreal MAX_ZOOM = 10.0;
    static constexpr qreal ZOOM_STEP = 1.2;

    void updateTrackPositions();
    int getTrackIndexAtPosition(const QPointF& scenePos) const;
    qreal getTimeAtPosition(qreal x) const;
};
