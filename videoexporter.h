#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <memory>
#include "exportsettings.h"

class VideoExporter : public QObject {
    Q_OBJECT

public:
    explicit VideoExporter(QObject* parent = nullptr);
    ~VideoExporter();
    
    // Export settings
    void setExportSettings(const ExportSettings& settings) { exportSettings = settings; }
    const ExportSettings& getExportSettings() const { return exportSettings; }
    
    // Export operations
    bool startExport(const QString& inputFile, const QString& outputFile);
    void cancelExport();
    
    // Status
    bool isExporting() const { return exportProcess && exportProcess->state() != QProcess::NotRunning; }
    double getProgress() const { return progress; }
    QString getLastError() const { return lastError; }
    
    // Get estimated file size
    qint64 estimateFileSize() const;
    
    // Generate preview frame
    bool generatePreview(const QString& inputFile, const QString& outputFile, 
                        double timestamp);

signals:
    void exportStarted();
    void exportProgress(double percent);
    void exportFinished(bool success);
    void exportError(const QString& error);
    void exportCancelled();

private slots:
    void handleProcessOutput();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void updateProgress();

private:
    ExportSettings exportSettings;
    std::unique_ptr<QProcess> exportProcess;
    QTimer progressTimer;
    double progress;
    QString lastError;
    
    // Helper functions
    QStringList buildFFmpegCommand(const QString& inputFile, 
                                  const QString& outputFile) const;
    double parseDuration(const QString& output) const;
    double parseTime(const QString& output) const;
    void reportError(const QString& error);
};
