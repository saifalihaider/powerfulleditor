#pragma once

#include <QGraphicsRectItem>
#include <QPainter>

class TimelineRuler : public QGraphicsRectItem {
public:
    explicit TimelineRuler(QGraphicsItem* parent = nullptr);

    void setZoomLevel(qreal level);
    void setViewportRange(qreal startTime, qreal duration);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    qreal zoomLevel;
    qreal viewportStartTime;
    qreal viewportDuration;

    static constexpr qreal RULER_HEIGHT = 30.0;
    static constexpr qreal MAJOR_TICK_HEIGHT = 15.0;
    static constexpr qreal MINOR_TICK_HEIGHT = 8.0;
    
    void drawTicks(QPainter* painter);
    QString formatTime(qreal seconds) const;
};
