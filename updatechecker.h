#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <memory>

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    struct Version {
        int major;
        int minor;
        int patch;
        QString toString() const;
        bool operator>(const Version& other) const;
    };

    struct UpdateInfo {
        Version version;
        QString downloadUrl;
        QString releaseNotes;
        bool isRequired;
    };

    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    // Update checking
    void checkForUpdates(bool silent = false);
    void setAutomaticChecks(bool enable, int intervalHours = 24);
    
    // Current version info
    Version getCurrentVersion() const;
    Version getLatestVersion() const;
    bool isUpdateAvailable() const;
    
    // Update settings
    void setUpdateUrl(const QString& url);
    void setProxySettings(const QNetworkProxy& proxy);

signals:
    void updateAvailable(const UpdateInfo& info);
    void noUpdateAvailable();
    void checkFailed(const QString& error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadComplete(const QString& filePath);
    void downloadFailed(const QString& error);

public slots:
    void downloadUpdate();
    void cancelDownload();

private slots:
    void handleNetworkReply(QNetworkReply* reply);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleAutoCheckTimer();

private:
    std::unique_ptr<QNetworkAccessManager> networkManager;
    QTimer autoCheckTimer;
    QString updateUrl;
    UpdateInfo latestUpdate;
    QNetworkReply* currentDownload;
    bool silent;

    // Helper functions
    Version parseVersionString(const QString& version) const;
    void processUpdateResponse(const QByteArray& data);
    QString getDownloadPath() const;
    void startDownload(const QString& url);
};
