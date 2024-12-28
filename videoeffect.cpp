#include "videoeffect.h"

VideoEffect::VideoEffect(EffectType type) : type(type) {}

QString VideoEffect::getName() const {
    switch (type) {
        case EffectType::Brightness: return "Brightness";
        case EffectType::Contrast: return "Contrast";
        case EffectType::Saturation: return "Saturation";
        case EffectType::Blur: return "Blur";
        case EffectType::Sharpen: return "Sharpen";
        case EffectType::Grayscale: return "Grayscale";
        case EffectType::Fade: return "Fade";
        default: return "Unknown Effect";
    }
}

void VideoEffect::setParameter(const QString& name, double value) {
    if (parameters.contains(name)) {
        parameters[name] = value;
    }
}

double VideoEffect::getParameter(const QString& name) const {
    return parameters.value(name, 0.0);
}

QString VideoEffect::getFFmpegFilter() const {
    return QString();  // Base class returns empty filter
}

std::unique_ptr<VideoEffect> VideoEffect::clone() const {
    std::unique_ptr<VideoEffect> newEffect;
    
    switch (type) {
        case EffectType::Brightness:
            newEffect = std::make_unique<BrightnessEffect>();
            break;
        case EffectType::Contrast:
            newEffect = std::make_unique<ContrastEffect>();
            break;
        case EffectType::Blur:
            newEffect = std::make_unique<BlurEffect>();
            break;
        case EffectType::Sharpen:
            newEffect = std::make_unique<SharpenEffect>();
            break;
        case EffectType::Fade:
            newEffect = std::make_unique<FadeEffect>();
            break;
        default:
            return nullptr;
    }
    
    // Copy parameters
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        newEffect->setParameter(it.key(), it.value());
    }
    
    return newEffect;
}

void VideoEffect::addParameter(const QString& name, double defaultValue) {
    parameters[name] = defaultValue;
}
