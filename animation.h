#pragma once

#include <QObject>
#include <QList>
#include <memory>
#include "keyframe.h"

class Animation : public QObject {
    Q_OBJECT

public:
    explicit Animation(KeyframeType type, QObject* parent = nullptr);
    
    // Keyframe management
    void addKeyframe(double time, const QVariant& value);
    void removeKeyframe(int index);
    void updateKeyframe(int index, double time, const QVariant& value);
    const QList<Keyframe>& getKeyframes() const { return keyframes; }
    
    // Get interpolated value at a specific time
    QVariant getValueAtTime(double time) const;
    
    // Generate FFmpeg filter string for this animation
    QString getFFmpegFilter(const QString& propertyName) const;
    
    // Get the time range of this animation
    double getStartTime() const;
    double getEndTime() const;
    double getDuration() const;

signals:
    void keyframesChanged();

private:
    KeyframeType type;
    QList<Keyframe> keyframes;
    
    // Helper function to find keyframes before and after a given time
    void findSurroundingKeyframes(double time, const Keyframe** before, 
                                const Keyframe** after) const;
};
