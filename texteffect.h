#pragma once

#include <QString>
#include <QFont>
#include <QColor>
#include <QPointF>
#include <memory>

enum class TextAnimationType {
    None,
    FadeIn,
    FadeOut,
    SlideLeft,
    SlideRight,
    SlideUp,
    SlideDown,
    Zoom
};

class TextEffect {
public:
    TextEffect();
    
    // Text content and style
    QString getText() const { return text; }
    QFont getFont() const { return font; }
    QColor getColor() const { return color; }
    QPointF getPosition() const { return position; }
    
    void setText(const QString& text) { this->text = text; }
    void setFont(const QFont& font) { this->font = font; }
    void setColor(const QColor& color) { this->color = color; }
    void setPosition(const QPointF& pos) { position = pos; }
    
    // Animation properties
    TextAnimationType getAnimationType() const { return animationType; }
    double getAnimationDuration() const { return animationDuration; }
    double getAnimationDelay() const { return animationDelay; }
    
    void setAnimationType(TextAnimationType type) { animationType = type; }
    void setAnimationDuration(double duration) { animationDuration = duration; }
    void setAnimationDelay(double delay) { animationDelay = delay; }
    
    // Timeline properties
    double getStartTime() const { return startTime; }
    double getDuration() const { return duration; }
    
    void setStartTime(double time) { startTime = time; }
    void setDuration(double dur) { duration = dur; }
    
    // Generate FFmpeg filter string
    QString getFFmpegFilter(int videoWidth, int videoHeight) const;
    
    // Clone this effect
    std::unique_ptr<TextEffect> clone() const;

private:
    QString text;
    QFont font;
    QColor color;
    QPointF position;  // Normalized coordinates (0-1)
    
    TextAnimationType animationType;
    double animationDuration;
    double animationDelay;
    
    double startTime;
    double duration;
    
    // Helper functions for FFmpeg filter generation
    QString generatePositionExpression(int videoWidth, int videoHeight) const;
    QString generateAlphaExpression() const;
    QString generateFontString() const;
};
