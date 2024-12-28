#include "audioeffect.h"

AudioEffect::AudioEffect(AudioEffectType type) : type(type) {}

QString AudioEffect::getName() const {
    switch (type) {
        case AudioEffectType::Volume: return "Volume";
        case AudioEffectType::Fade: return "Fade";
        case AudioEffectType::Equalizer: return "Equalizer";
        case AudioEffectType::NoiseReduction: return "Noise Reduction";
        case AudioEffectType::Balance: return "Balance";
        default: return "Unknown Effect";
    }
}

void AudioEffect::setParameter(const QString& name, double value) {
    if (parameters.contains(name)) {
        parameters[name] = value;
    }
}

double AudioEffect::getParameter(const QString& name) const {
    return parameters.value(name, 0.0);
}

QString AudioEffect::getFFmpegFilter() const {
    return QString();  // Base class returns empty filter
}

std::unique_ptr<AudioEffect> AudioEffect::clone() const {
    std::unique_ptr<AudioEffect> newEffect;
    
    switch (type) {
        case AudioEffectType::Volume:
            newEffect = std::make_unique<VolumeEffect>();
            break;
        case AudioEffectType::Fade:
            newEffect = std::make_unique<AudioFadeEffect>();
            break;
        case AudioEffectType::Equalizer:
            newEffect = std::make_unique<EqualizerEffect>();
            break;
        case AudioEffectType::NoiseReduction:
            newEffect = std::make_unique<NoiseReductionEffect>();
            break;
        case AudioEffectType::Balance:
            newEffect = std::make_unique<BalanceEffect>();
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

void AudioEffect::addParameter(const QString& name, double defaultValue) {
    parameters[name] = defaultValue;
}
