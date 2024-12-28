#pragma once

#include <QObject>
#include <QList>
#include <memory>
#include "texteffect.h"

class TextManager : public QObject {
    Q_OBJECT

public:
    explicit TextManager(QObject* parent = nullptr);
    
    // Text effect management
    void addTextEffect(std::unique_ptr<TextEffect> effect);
    void removeTextEffect(int index);
    void updateTextEffect(int index, const TextEffect& effect);
    const QList<std::unique_ptr<TextEffect>>& getTextEffects() const { return textEffects; }
    
    // Generate FFmpeg filter string for all text effects
    QString generateFilterString(int videoWidth, int videoHeight) const;
    
    // Apply text effects to video
    bool applyTextEffects(const QString& inputFile, const QString& outputFile);
    
    // Generate preview frame
    bool generatePreviewFrame(const QString& inputFile, const QString& outputFile,
                            double timestamp, int width, int height);

signals:
    void textEffectsChanged();
    void processingStarted();
    void processingFinished(bool success);
    void progressUpdated(int percent);

private:
    QList<std::unique_ptr<TextEffect>> textEffects;
    
    // Helper function to run FFmpeg commands
    bool runFFmpegCommand(const QString& command);
};
