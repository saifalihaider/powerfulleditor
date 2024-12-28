#include "timelineruler.h"
#include <QFontMetrics>

TimelineRuler::TimelineRuler(QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , zoomLevel(1.0)
    , viewportStartTime(0.0)
    , viewportDuration(60.0)
{
    setRect(0, 0, 2000, RULER_HEIGHT);
}

void TimelineRuler::setZoomLevel(qreal level) {
    zoomLevel = level;
    update();
}

void TimelineRuler::setViewportRange(qreal startTime, qreal duration) {
    viewportStartTime = startTime;
    viewportDuration = duration;
    update();
}

void TimelineRuler::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Draw background
    painter->fillRect(rect(), QBrush(Qt::lightGray));
    painter->setPen(Qt::black);

    drawTicks(painter);
}

void TimelineRuler::drawTicks(QPainter* painter) {
    const qreal pixelsPerSecond = TimelineClip::PIXELS_PER_SECOND * zoomLevel;
    const qreal totalWidth = rect().width();
    
    // Calculate appropriate tick intervals based on zoom level
    qreal majorTickInterval;
    if (zoomLevel > 2.0) {
        majorTickInterval = 1.0; // 1 second
    } else if (zoomLevel > 0.5) {
        majorTickInterval = 5.0; // 5 seconds
    } else {
        majorTickInterval = 10.0; // 10 seconds
    }
    
    const qreal minorTickInterval = majorTickInterval / 5;
    
    // Draw ticks
    for (qreal time = viewportStartTime; time <= viewportStartTime + viewportDuration; time += minorTickInterval) {
        qreal x = time * pixelsPerSecond;
        
        if (x < 0 || x > totalWidth) continue;
        
        bool isMajorTick = std::fmod(time, majorTickInterval) < 0.001;
        qreal tickHeight = isMajorTick ? MAJOR_TICK_HEIGHT : MINOR_TICK_HEIGHT;
        
        painter->drawLine(QPointF(x, rect().height()),
                         QPointF(x, rect().height() - tickHeight));
        
        if (isMajorTick) {
            QString timeText = formatTime(time);
            QFontMetrics fm(painter->font());
            QRect textRect = fm.boundingRect(timeText);
            painter->drawText(QPointF(x - textRect.width() / 2,
                                    rect().height() - MAJOR_TICK_HEIGHT - 2),
                            timeText);
        }
    }
}

QString TimelineRuler::formatTime(qreal seconds) const {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int millisecs = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);
    
    if (zoomLevel > 2.0) {
        // Show milliseconds when zoomed in
        return QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(millisecs, 3, 10, QChar('0'));
    } else {
        // Show only minutes and seconds when zoomed out
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}
