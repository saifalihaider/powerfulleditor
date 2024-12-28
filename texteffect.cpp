#include "texteffect.h"

TextEffect::TextEffect()
    : text("Sample Text")
    , font("Arial", 24)
    , color(Qt::white)
    , position(0.5, 0.5)  // Center of the screen
    , animationType(TextAnimationType::None)
    , animationDuration(1.0)
    , animationDelay(0.0)
    , startTime(0.0)
    , duration(5.0)
{
}

QString TextEffect::getFFmpegFilter(int videoWidth, int videoHeight) const {
    QString filter = QString("drawtext=");
    
    // Basic text properties
    filter += QString("text='%1':").arg(text.replace("'", "\\'"));
    filter += generateFontString();
    filter += QString("fontcolor=%1@%2:")
        .arg(color.name())
        .arg(color.alpha());
    
    // Position
    filter += generatePositionExpression(videoWidth, videoHeight);
    
    // Animation (alpha/transparency)
    filter += generateAlphaExpression();
    
    // Enable expression based on timeline
    filter += QString("enable='between(t,%1,%2)'")
        .arg(startTime)
        .arg(startTime + duration);
    
    return filter;
}

QString TextEffect::generatePositionExpression(int videoWidth, int videoHeight) const {
    QString xPos, yPos;
    
    // Base position calculation
    double normalizedX = position.x();
    double normalizedY = position.y();
    
    // Add animation offsets if needed
    switch (animationType) {
        case TextAnimationType::SlideLeft:
            xPos = QString("x='w*%1+w*(1-min((t-%2)/%3,1))':")
                .arg(normalizedX)
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            yPos = QString("y='h*%1':").arg(normalizedY);
            break;
            
        case TextAnimationType::SlideRight:
            xPos = QString("x='w*%1-w*(1-min((t-%2)/%3,1))':")
                .arg(normalizedX)
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            yPos = QString("y='h*%1':").arg(normalizedY);
            break;
            
        case TextAnimationType::SlideUp:
            xPos = QString("x='w*%1':").arg(normalizedX);
            yPos = QString("y='h*%1+h*(1-min((t-%2)/%3,1))':")
                .arg(normalizedY)
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            break;
            
        case TextAnimationType::SlideDown:
            xPos = QString("x='w*%1':").arg(normalizedX);
            yPos = QString("y='h*%1-h*(1-min((t-%2)/%3,1))':")
                .arg(normalizedY)
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            break;
            
        case TextAnimationType::Zoom:
            // Implement zoom by scaling font size
            xPos = QString("x='w*%1':").arg(normalizedX);
            yPos = QString("y='h*%1':").arg(normalizedY);
            break;
            
        default:
            xPos = QString("x='w*%1':").arg(normalizedX);
            yPos = QString("y='h*%1':").arg(normalizedY);
    }
    
    return xPos + yPos;
}

QString TextEffect::generateAlphaExpression() const {
    QString alphaExpr;
    
    switch (animationType) {
        case TextAnimationType::FadeIn:
            alphaExpr = QString("alpha='min((t-%1)/%2,1)':")
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            break;
            
        case TextAnimationType::FadeOut:
            alphaExpr = QString("alpha='1-min((t-(%1))/%2,1)':")
                .arg(startTime + duration - animationDuration)
                .arg(animationDuration);
            break;
            
        case TextAnimationType::Zoom:
            // Combine alpha with font size scaling
            alphaExpr = QString("fontsize='%1*min((t-%2)/%3,1)':")
                .arg(font.pointSize() * 2)
                .arg(startTime + animationDelay)
                .arg(animationDuration);
            break;
            
        default:
            alphaExpr = "alpha='1':";
    }
    
    return alphaExpr;
}

QString TextEffect::generateFontString() const {
    QString fontString = QString("fontfile='%1':fontsize=%2:")
        .arg(font.family())
        .arg(font.pointSize());
    
    if (font.bold()) {
        fontString += "bold=1:";
    }
    if (font.italic()) {
        fontString += "italic=1:";
    }
    
    return fontString;
}

std::unique_ptr<TextEffect> TextEffect::clone() const {
    auto newEffect = std::make_unique<TextEffect>();
    
    newEffect->text = text;
    newEffect->font = font;
    newEffect->color = color;
    newEffect->position = position;
    newEffect->animationType = animationType;
    newEffect->animationDuration = animationDuration;
    newEffect->animationDelay = animationDelay;
    newEffect->startTime = startTime;
    newEffect->duration = duration;
    
    return newEffect;
}
