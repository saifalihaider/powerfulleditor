#pragma once

#include <QObject>
#include <QHash>
#include <QSize>
#include <QProcess>
#include <memory>
#include <optional>

struct ProxySettings {
    QSize resolution{640, 360};  // Default proxy resolution
    int bitrate{1000};          // Default bitrate in kbps
    int frameRate{30};          // Default frame rate
    QString preset{"ultrafast"}; // FFmpeg preset for quick proxy generation
};

class ProxyManager : public QObject {
    Q_OBJECT

public:
    explicit ProxyManager(const QString& cacheDir, QObject* parent = nullptr);
    ~ProxyManager();
    
    // Proxy file management
    bool createProxy(const QString& sourceFile);
    QString getProxyPath(const QString& sourceFile) const;
    bool hasProxy(const QString& sourceFile) const;
    void removeProxy(const QString& sourceFile);
    void clearAllProxies();
    
    // Settings
    void setProxySettings(const ProxySettings& settings) { proxySettings = settings; }
    const ProxySettings& getProxySettings() const { return proxySettings; }
    
    // Cache management
    void setCacheDirectory(const QString& dir);
    QString getCacheDirectory() const { return cacheDir; }
    qint64 getCacheSize() const;
    void clearCache();
    
    // Status
    bool isGeneratingProxy() const;
    double getProgress() const { return progress; }
    QString getLastError() const { return lastError; }

signals:
    void proxyGenerationStarted(const QString& sourceFile);
    void proxyGenerationProgress(const QString& sourceFile, double percent);
    void proxyGenerationFinished(const QString& sourceFile, bool success);
    void proxyGenerationError(const QString& sourceFile, const QString& error);
    void cacheCleared();

private slots:
    void handleProcessOutput();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString cacheDir;
    ProxySettings proxySettings;
    std::unique_ptr<QProcess> proxyProcess;
    QString currentSourceFile;
    double progress;
    QString lastError;
    QHash<QString, QString> proxyFiles;  // source file -> proxy file mapping
    
    // Helper functions
    QString generateProxyPath(const QString& sourceFile) const;
    QStringList buildFFmpegCommand(const QString& sourceFile, 
                                  const QString& proxyFile) const;
    bool ensureCacheDirectory() const;
    void reportError(const QString& error);
    double parseDuration(const QString& output) const;
    double parseTime(const QString& timeStr) const;
    
    // Hardware acceleration support
    std::optional<QString> detectHardwareAcceleration() const;
};
