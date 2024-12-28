#pragma once

#include <QString>
#include <QMap>
#include <memory>

enum class AudioEffectType {
    Volume,
    Fade,
    Equalizer,
    NoiseReduction,
    Balance
};

class AudioEffect {
public:
    AudioEffect(AudioEffectType type);
    virtual ~AudioEffect() = default;

    AudioEffectType getType() const { return type; }
    QString getName() const;
    
    void setParameter(const QString& name, double value);
    double getParameter(const QString& name) const;
    
    // Get the FFmpeg filter string for this effect
    virtual QString getFFmpegFilter() const;
    
    // Clone this effect
    virtual std::unique_ptr<AudioEffect> clone() const;

protected:
    AudioEffectType type;
    QMap<QString, double> parameters;
    
    void addParameter(const QString& name, double defaultValue);
};

// Specific audio effect implementations
class VolumeEffect : public AudioEffect {
public:
    VolumeEffect() : AudioEffect(AudioEffectType::Volume) {
        addParameter("volume", 1.0);  // 0.0 to 2.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("volume=%1").arg(getParameter("volume"));
    }
};

class AudioFadeEffect : public AudioEffect {
public:
    AudioFadeEffect() : AudioEffect(AudioEffectType::Fade) {
        addParameter("start_time", 0.0);
        addParameter("duration", 1.0);
        addParameter("type", 0.0);  // 0 = fade in, 1 = fade out
    }
    
    QString getFFmpegFilter() const override {
        QString fadeType = getParameter("type") < 0.5 ? "in" : "out";
        return QString("afade=t=%1:st=%2:d=%3")
            .arg(fadeType)
            .arg(getParameter("start_time"))
            .arg(getParameter("duration"));
    }
};

class EqualizerEffect : public AudioEffect {
public:
    EqualizerEffect() : AudioEffect(AudioEffectType::Equalizer) {
        // Add frequency bands
        addParameter("low", 1.0);    // 0.0 to 2.0
        addParameter("mid", 1.0);    // 0.0 to 2.0
        addParameter("high", 1.0);   // 0.0 to 2.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("equalizer=f=100:t=h:w=200:g=%1,equalizer=f=1000:t=h:w=200:g=%2,"
                      "equalizer=f=10000:t=h:w=200:g=%3")
            .arg(getParameter("low"))
            .arg(getParameter("mid"))
            .arg(getParameter("high"));
    }
};

class NoiseReductionEffect : public AudioEffect {
public:
    NoiseReductionEffect() : AudioEffect(AudioEffectType::NoiseReduction) {
        addParameter("amount", 0.5);  // 0.0 to 1.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("anlmdn=s=%1").arg(getParameter("amount"));
    }
};

class BalanceEffect : public AudioEffect {
public:
    BalanceEffect() : AudioEffect(AudioEffectType::Balance) {
        addParameter("balance", 0.0);  // -1.0 (left) to 1.0 (right)
    }
    
    QString getFFmpegFilter() const override {
        double balance = getParameter("balance");
        double leftVol = balance <= 0 ? 1.0 : 1.0 - balance;
        double rightVol = balance >= 0 ? 1.0 : 1.0 + balance;
        return QString("pan=stereo|c0=%1*c0|c1=%2*c1").arg(leftVol).arg(rightVol);
    }
};
