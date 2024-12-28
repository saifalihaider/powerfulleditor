#pragma once

#include <QVariant>
#include <QString>

enum class KeyframeType {
    Position,    // QPointF
    Scale,       // QPointF (x = width scale, y = height scale)
    Rotation,    // double
    Opacity,     // double
    Color        // QColor
};

class Keyframe {
public:
    Keyframe(KeyframeType type, double time, const QVariant& value);
    
    KeyframeType getType() const { return type; }
    double getTime() const { return time; }
    QVariant getValue() const { return value; }
    
    void setTime(double newTime) { time = newTime; }
    void setValue(const QVariant& newValue) { value = newValue; }
    
    // Interpolate between this keyframe and another
    QVariant interpolate(const Keyframe& other, double currentTime) const;
    
    // Generate FFmpeg expression for this keyframe type
    QString getFFmpegExpression(const QString& propertyName, 
                              const Keyframe* nextKeyframe = nullptr) const;

private:
    KeyframeType type;
    double time;
    QVariant value;
    
    // Helper functions for interpolation
    static QVariant interpolatePosition(const QVariant& start, const QVariant& end, double factor);
    static QVariant interpolateScale(const QVariant& start, const QVariant& end, double factor);
    static QVariant interpolateRotation(const QVariant& start, const QVariant& end, double factor);
    static QVariant interpolateOpacity(const QVariant& start, const QVariant& end, double factor);
    static QVariant interpolateColor(const QVariant& start, const QVariant& end, double factor);
};
