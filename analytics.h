#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QQueue>
#include <memory>

class Analytics : public QObject {
    Q_OBJECT

public:
    enum class EventType {
        AppStart,
        AppExit,
        VideoImport,
        VideoExport,
        EffectApplied,
        Error,
        Performance,
        UserAction,
        FeatureUsage
    };

    struct Event {
        EventType type;
        QString name;
        QJsonObject properties;
        qint64 timestamp;
    };

    static Analytics& instance();
    
    // Event tracking
    void trackEvent(EventType type, const QString& name, 
                   const QJsonObject& properties = QJsonObject());
    void trackError(const QString& error, const QString& stackTrace = QString());
    void trackPerformance(const QString& operation, qint64 duration);
    void trackFeatureUsage(const QString& feature);
    
    // Session management
    void startSession();
    void endSession();
    
    // Configuration
    void setEnabled(bool enabled);
    void setUserId(const QString& userId);
    void setCustomProperty(const QString& key, const QVariant& value);
    
    // Crash reporting
    void enableCrashReporting(bool enable);
    void reportCrash(const QString& reason, const QString& stackTrace);

signals:
    void eventTracked(const Event& event);
    void errorOccurred(const QString& error);

private:
    explicit Analytics(QObject* parent = nullptr);
    ~Analytics();
    
    // Network management
    void sendEvents();
    void handleNetworkReply(QNetworkReply* reply);
    
    // Event queue management
    void queueEvent(const Event& event);
    void processEventQueue();
    
    // Persistent storage
    void saveEventQueue();
    void loadEventQueue();
    
    // Helper functions
    QJsonObject createBaseProperties() const;
    QString getDeviceInfo() const;
    QString getOSInfo() const;
    
    // Member variables
    std::unique_ptr<QNetworkAccessManager> networkManager;
    QQueue<Event> eventQueue;
    QTimer flushTimer;
    QString userId;
    bool enabled;
    bool crashReportingEnabled;
    QJsonObject customProperties;
    QString sessionId;
    qint64 sessionStartTime;
    
    // Constants
    static const QString API_ENDPOINT;
    static const int FLUSH_INTERVAL;
    static const int MAX_QUEUE_SIZE;
    static const int MAX_BATCH_SIZE;
};
