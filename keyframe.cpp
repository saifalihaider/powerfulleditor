#include "keyframe.h"
#include <QPointF>
#include <QColor>
#include <cmath>

Keyframe::Keyframe(KeyframeType type, double time, const QVariant& value)
    : type(type)
    , time(time)
    , value(value)
{
}

QVariant Keyframe::interpolate(const Keyframe& other, double currentTime) const {
    if (type != other.type) return value;
    
    double totalTime = other.time - time;
    if (totalTime <= 0) return value;
    
    double factor = (currentTime - time) / totalTime;
    factor = qBound(0.0, factor, 1.0);
    
    switch (type) {
        case KeyframeType::Position:
            return interpolatePosition(value, other.value, factor);
        case KeyframeType::Scale:
            return interpolateScale(value, other.value, factor);
        case KeyframeType::Rotation:
            return interpolateRotation(value, other.value, factor);
        case KeyframeType::Opacity:
            return interpolateOpacity(value, other.value, factor);
        case KeyframeType::Color:
            return interpolateColor(value, other.value, factor);
        default:
            return value;
    }
}

QString Keyframe::getFFmpegExpression(const QString& propertyName, 
                                    const Keyframe* nextKeyframe) const {
    QString expr;
    
    switch (type) {
        case KeyframeType::Position: {
            QPointF pos = value.toPointF();
            if (nextKeyframe) {
                QPointF nextPos = nextKeyframe->value.toPointF();
                expr = QString("%1='if(between(t,%2,%3),lerp(%4,%5,(t-%2)/(%3-%2)),"
                             "if(lt(t,%2),%4,if(gt(t,%3),%5,%4)))'")
                    .arg(propertyName)
                    .arg(time)
                    .arg(nextKeyframe->time)
                    .arg(pos.x())
                    .arg(nextPos.x());
            } else {
                expr = QString("%1=%2").arg(propertyName).arg(pos.x());
            }
            break;
        }
        
        case KeyframeType::Scale: {
            QPointF scale = value.toPointF();
            if (nextKeyframe) {
                QPointF nextScale = nextKeyframe->value.toPointF();
                expr = QString("scale='if(between(t,%1,%2),lerp(%3,%4,(t-%1)/(%2-%1)),"
                             "if(lt(t,%1),%3,if(gt(t,%2),%4,%3)))'")
                    .arg(time)
                    .arg(nextKeyframe->time)
                    .arg(scale.x())
                    .arg(nextScale.x());
            } else {
                expr = QString("scale=%1:%2").arg(scale.x()).arg(scale.y());
            }
            break;
        }
        
        case KeyframeType::Rotation: {
            double angle = value.toDouble();
            if (nextKeyframe) {
                double nextAngle = nextKeyframe->value.toDouble();
                expr = QString("rotate='if(between(t,%1,%2),lerp(%3,%4,(t-%1)/(%2-%1)),"
                             "if(lt(t,%1),%3,if(gt(t,%2),%4,%3)))'")
                    .arg(time)
                    .arg(nextKeyframe->time)
                    .arg(angle)
                    .arg(nextAngle);
            } else {
                expr = QString("rotate=%1").arg(angle);
            }
            break;
        }
        
        case KeyframeType::Opacity: {
            double opacity = value.toDouble();
            if (nextKeyframe) {
                double nextOpacity = nextKeyframe->value.toDouble();
                expr = QString("alpha='if(between(t,%1,%2),lerp(%3,%4,(t-%1)/(%2-%1)),"
                             "if(lt(t,%1),%3,if(gt(t,%2),%4,%3)))'")
                    .arg(time)
                    .arg(nextKeyframe->time)
                    .arg(opacity)
                    .arg(nextOpacity);
            } else {
                expr = QString("alpha=%1").arg(opacity);
            }
            break;
        }
        
        case KeyframeType::Color: {
            QColor color = value.value<QColor>();
            if (nextKeyframe) {
                QColor nextColor = nextKeyframe->value.value<QColor>();
                expr = QString("colorize='if(between(t,%1,%2),lerp(%3,%4,(t-%1)/(%2-%1)),"
                             "if(lt(t,%1),%3,if(gt(t,%2),%4,%3)))'")
                    .arg(time)
                    .arg(nextKeyframe->time)
                    .arg(color.name())
                    .arg(nextColor.name());
            } else {
                expr = QString("colorize=%1").arg(color.name());
            }
            break;
        }
    }
    
    return expr;
}

QVariant Keyframe::interpolatePosition(const QVariant& start, const QVariant& end, double factor) {
    QPointF startPos = start.toPointF();
    QPointF endPos = end.toPointF();
    return QVariant(QPointF(
        startPos.x() + (endPos.x() - startPos.x()) * factor,
        startPos.y() + (endPos.y() - startPos.y()) * factor
    ));
}

QVariant Keyframe::interpolateScale(const QVariant& start, const QVariant& end, double factor) {
    QPointF startScale = start.toPointF();
    QPointF endScale = end.toPointF();
    return QVariant(QPointF(
        startScale.x() + (endScale.x() - startScale.x()) * factor,
        startScale.y() + (endScale.y() - startScale.y()) * factor
    ));
}

QVariant Keyframe::interpolateRotation(const QVariant& start, const QVariant& end, double factor) {
    double startAngle = start.toDouble();
    double endAngle = end.toDouble();
    return QVariant(startAngle + (endAngle - startAngle) * factor);
}

QVariant Keyframe::interpolateOpacity(const QVariant& start, const QVariant& end, double factor) {
    double startOpacity = start.toDouble();
    double endOpacity = end.toDouble();
    return QVariant(startOpacity + (endOpacity - startOpacity) * factor);
}

QVariant Keyframe::interpolateColor(const QVariant& start, const QVariant& end, double factor) {
    QColor startColor = start.value<QColor>();
    QColor endColor = end.value<QColor>();
    return QVariant(QColor(
        startColor.red() + (endColor.red() - startColor.red()) * factor,
        startColor.green() + (endColor.green() - startColor.green()) * factor,
        startColor.blue() + (endColor.blue() - startColor.blue()) * factor,
        startColor.alpha() + (endColor.alpha() - startColor.alpha()) * factor
    ));
}
