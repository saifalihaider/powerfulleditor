#pragma once

#include <QString>
#include <QMap>
#include <memory>

enum class EffectType {
    Brightness,
    Contrast,
    Saturation,
    Blur,
    Sharpen,
    Grayscale,
    Fade
};

class VideoEffect {
public:
    VideoEffect(EffectType type);
    virtual ~VideoEffect() = default;

    EffectType getType() const { return type; }
    QString getName() const;
    
    void setParameter(const QString& name, double value);
    double getParameter(const QString& name) const;
    
    // Get the FFmpeg filter string for this effect
    virtual QString getFFmpegFilter() const;
    
    // Clone this effect
    virtual std::unique_ptr<VideoEffect> clone() const;

protected:
    EffectType type;
    QMap<QString, double> parameters;
    
    void addParameter(const QString& name, double defaultValue);
};

// Specific effect implementations
class BrightnessEffect : public VideoEffect {
public:
    BrightnessEffect() : VideoEffect(EffectType::Brightness) {
        addParameter("brightness", 0.0);  // -1.0 to 1.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("eq=brightness=%1").arg(getParameter("brightness"));
    }
};

class ContrastEffect : public VideoEffect {
public:
    ContrastEffect() : VideoEffect(EffectType::Contrast) {
        addParameter("contrast", 1.0);    // 0.0 to 2.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("eq=contrast=%1").arg(getParameter("contrast"));
    }
};

class BlurEffect : public VideoEffect {
public:
    BlurEffect() : VideoEffect(EffectType::Blur) {
        addParameter("radius", 5.0);      // 1.0 to 20.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("boxblur=%1:1").arg(getParameter("radius"));
    }
};

class SharpenEffect : public VideoEffect {
public:
    SharpenEffect() : VideoEffect(EffectType::Sharpen) {
        addParameter("amount", 1.0);      // 0.0 to 5.0
    }
    
    QString getFFmpegFilter() const override {
        return QString("unsharp=%1:5:0:5:0").arg(getParameter("amount"));
    }
};

class FadeEffect : public VideoEffect {
public:
    FadeEffect() : VideoEffect(EffectType::Fade) {
        addParameter("start_time", 0.0);
        addParameter("duration", 1.0);
        addParameter("type", 0.0);  // 0 = fade in, 1 = fade out
    }
    
    QString getFFmpegFilter() const override {
        QString fadeType = getParameter("type") < 0.5 ? "in" : "out";
        return QString("fade=t=%1:st=%2:d=%3")
            .arg(fadeType)
            .arg(getParameter("start_time"))
            .arg(getParameter("duration"));
    }
};
