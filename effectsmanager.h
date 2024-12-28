#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <memory>
#include "videoeffect.h"

class EffectsManager : public QObject {
    Q_OBJECT

public:
    explicit EffectsManager(QObject* parent = nullptr);
    
    // Effect management
    void addEffect(std::unique_ptr<VideoEffect> effect);
    void removeEffect(int index);
    void clearEffects();
    
    // Get all effects
    const QList<std::unique_ptr<VideoEffect>>& getEffects() const { return effects; }
    
    // Generate FFmpeg filter string for all effects
    QString generateFilterString() const;
    
    // Apply effects to a video file
    bool applyEffects(const QString& inputFile, const QString& outputFile);
    
    // Generate a preview frame with current effects
    bool generatePreviewFrame(const QString& inputFile, const QString& outputFile, double timestamp);

signals:
    void effectsChanged();
    void processingStarted();
    void processingFinished(bool success);
    void progressUpdated(int percent);

private:
    QList<std::unique_ptr<VideoEffect>> effects;
    
    // Helper function to run FFmpeg commands
    bool runFFmpegCommand(const QString& command);
};
