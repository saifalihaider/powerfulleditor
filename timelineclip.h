#pragma once

#include <QGraphicsRectItem>
#include <QString>
#include <QBrush>

class TimelineClip : public QGraphicsRectItem {
public:
    TimelineClip(const QString& filePath, qreal startTime, qreal duration, 
                 QGraphicsItem* parent = nullptr);

    QString getFilePath() const { return filePath; }
    qreal getStartTime() const { return startTime; }
    qreal getDuration() const { return duration; }

    void setStartTime(qreal time);
    void setDuration(qreal newDuration);
    void updateVisual();

    // Constants for clip appearance
    static constexpr qreal MIN_CLIP_WIDTH = 10.0;
    static constexpr qreal PIXELS_PER_SECOND = 50.0;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString filePath;
    qreal startTime;
    qreal duration;
    QPointF dragStartPos;
    bool isDragging;

    QBrush getClipBrush() const;
};
