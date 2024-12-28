#include "timelineclip.h"
#include <QGraphicsSceneMouseEvent>
#include <QFileInfo>

TimelineClip::TimelineClip(const QString& filePath, qreal startTime, qreal duration,
                         QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , filePath(filePath)
    , startTime(startTime)
    , duration(duration)
    , isDragging(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    updateVisual();
}

void TimelineClip::setStartTime(qreal time) {
    startTime = time;
    updateVisual();
}

void TimelineClip::setDuration(qreal newDuration) {
    duration = qMax(newDuration, MIN_CLIP_WIDTH / PIXELS_PER_SECOND);
    updateVisual();
}

void TimelineClip::updateVisual() {
    qreal width = duration * PIXELS_PER_SECOND;
    setRect(startTime * PIXELS_PER_SECOND, 0, width, parentItem()->boundingRect().height());
    setBrush(getClipBrush());
    setPen(QPen(Qt::black));
}

QBrush TimelineClip::getClipBrush() const {
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    if (extension == "mp4" || extension == "avi" || extension == "mov") {
        return QBrush(QColor(65, 105, 225));  // Video clip color
    } else if (extension == "mp3" || extension == "wav") {
        return QBrush(QColor(50, 205, 50));   // Audio clip color
    } else {
        return QBrush(QColor(255, 140, 0));   // Text/other clip color
    }
}

void TimelineClip::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        dragStartPos = event->pos();
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void TimelineClip::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (isDragging) {
        QPointF newPos = mapToParent(event->pos() - dragStartPos);
        newPos.setY(0);  // Constrain vertical movement
        setPos(newPos);
    }
    QGraphicsRectItem::mouseMoveEvent(event);
}

void TimelineClip::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        // Snap to grid or handle collision here
        setStartTime(pos().x() / PIXELS_PER_SECOND);
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}
