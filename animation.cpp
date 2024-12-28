#include "animation.h"
#include <algorithm>

Animation::Animation(KeyframeType type, QObject* parent)
    : QObject(parent)
    , type(type)
{
}

void Animation::addKeyframe(double time, const QVariant& value) {
    // Find the position to insert the new keyframe
    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const Keyframe& kf, double t) { return kf.getTime() < t; });
    
    // If a keyframe already exists at this time, update it
    if (it != keyframes.end() && qFuzzyCompare(it->getTime(), time)) {
        it->setValue(value);
    } else {
        // Insert the new keyframe
        keyframes.insert(it, Keyframe(type, time, value));
    }
    
    emit keyframesChanged();
}

void Animation::removeKeyframe(int index) {
    if (index >= 0 && index < keyframes.size()) {
        keyframes.removeAt(index);
        emit keyframesChanged();
    }
}

void Animation::updateKeyframe(int index, double time, const QVariant& value) {
    if (index >= 0 && index < keyframes.size()) {
        // Remove the keyframe from its current position
        Keyframe kf = keyframes.takeAt(index);
        
        // Update its values
        kf.setTime(time);
        kf.setValue(value);
        
        // Find the new position to insert it
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
            [](const Keyframe& kf, double t) { return kf.getTime() < t; });
        
        // Insert at the new position
        keyframes.insert(it, kf);
        
        emit keyframesChanged();
    }
}

QVariant Animation::getValueAtTime(double time) const {
    if (keyframes.isEmpty()) {
        return QVariant();
    }
    
    // Find the keyframes before and after the given time
    const Keyframe *before = nullptr, *after = nullptr;
    findSurroundingKeyframes(time, &before, &after);
    
    if (!before) {
        // Before the first keyframe, return the first keyframe's value
        return keyframes.first().getValue();
    }
    
    if (!after) {
        // After the last keyframe, return the last keyframe's value
        return keyframes.last().getValue();
    }
    
    // Interpolate between the two keyframes
    return before->interpolate(*after, time);
}

QString Animation::getFFmpegFilter(const QString& propertyName) const {
    if (keyframes.isEmpty()) {
        return QString();
    }
    
    QStringList expressions;
    
    // Generate expressions for each keyframe pair
    for (int i = 0; i < keyframes.size() - 1; ++i) {
        expressions.append(keyframes[i].getFFmpegExpression(propertyName, &keyframes[i + 1]));
    }
    
    // Add the last keyframe's expression
    expressions.append(keyframes.last().getFFmpegExpression(propertyName));
    
    return expressions.join(",");
}

double Animation::getStartTime() const {
    return keyframes.isEmpty() ? 0.0 : keyframes.first().getTime();
}

double Animation::getEndTime() const {
    return keyframes.isEmpty() ? 0.0 : keyframes.last().getTime();
}

double Animation::getDuration() const {
    return getEndTime() - getStartTime();
}

void Animation::findSurroundingKeyframes(double time, const Keyframe** before, 
                                       const Keyframe** after) const {
    *before = nullptr;
    *after = nullptr;
    
    for (const auto& kf : keyframes) {
        if (kf.getTime() <= time) {
            *before = &kf;
        } else {
            *after = &kf;
            break;
        }
    }
}
