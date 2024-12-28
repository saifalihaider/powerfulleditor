#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QString>
#include <QImage>
#include <memory>

class BugReporter : public QObject {
    Q_OBJECT

public:
    struct BugReport {
        QString title;
        QString description;
        QString systemInfo;
        QString logContent;
        QString stackTrace;
        QImage screenshot;
        QString userEmail;
        QString reproSteps;
        QJsonObject metadata;
    };

    static BugReporter& instance();
    
    // Report submission
    void submitReport(const BugReport& report);
    void submitCrashReport(const QString& crashReason, 
                         const QString& stackTrace);
    
    // Configuration
    void setApiKey(const QString& key);
    void setEndpoint(const QString& url);
    void setUserEmail(const QString& email);
    
    // System information
    QString getSystemInfo() const;
    QString getLogContent() const;
    QImage captureScreenshot() const;
    
    // Error handling
    QString getLastError() const;
    void clearLastError();

signals:
    void reportSubmitted(bool success, const QString& reportId);
    void reportProgress(int percent);
    void errorOccurred(const QString& error);

private:
    explicit BugReporter(QObject* parent = nullptr);
    ~BugReporter();
    
    // Network operations
    void sendReport(const QJsonObject& reportData,
                   const QImage& screenshot = QImage());
    void handleNetworkReply(QNetworkReply* reply);
    
    // Helper functions
    QJsonObject createReportJson(const BugReport& report) const;
    bool validateReport(const BugReport& report) const;
    QString generateReportId() const;
    
    // File operations
    bool saveScreenshot(const QImage& screenshot,
                       const QString& reportId) const;
    QString getLogFilePath() const;
    void rotateLogs();
    
    // Member variables
    std::unique_ptr<QNetworkAccessManager> networkManager;
    QString apiKey;
    QString apiEndpoint;
    QString userEmail;
    QString lastError;
    
    // Constants
    static const QString DEFAULT_API_ENDPOINT;
    static const int MAX_SCREENSHOT_SIZE;
    static const int MAX_LOG_SIZE;
    static const int MAX_LOG_FILES;
};
