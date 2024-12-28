#include "analytics.h"
#include "version.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QSysInfo>
#include <QScreen>
#include <QGuiApplication>
#include <QUuid>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

const QString Analytics::API_ENDPOINT = "https://analytics.example.com/v1/events";
const int Analytics::FLUSH_INTERVAL = 30000; // 30 seconds
const int Analytics::MAX_QUEUE_SIZE = 1000;
const int Analytics::MAX_BATCH_SIZE = 50;

Analytics& Analytics::instance() {
    static Analytics instance;
    return instance;
}

Analytics::Analytics(QObject* parent)
    : QObject(parent)
    , networkManager(std::make_unique<QNetworkAccessManager>())
    , enabled(true)
    , crashReportingEnabled(true)
    , sessionStartTime(QDateTime::currentMSecsSinceEpoch())
{
    // Set up network manager
    connect(networkManager.get(), &QNetworkAccessManager::finished,
            this, &Analytics::handleNetworkReply);
    
    // Set up flush timer
    connect(&flushTimer, &QTimer::timeout,
            this, &Analytics::processEventQueue);
    flushTimer.setInterval(FLUSH_INTERVAL);
    flushTimer.start();
    
    // Load saved events
    loadEventQueue();
    
    // Start new session
    startSession();
}

Analytics::~Analytics() {
    endSession();
    saveEventQueue();
}

void Analytics::trackEvent(EventType type, const QString& name,
                         const QJsonObject& properties) {
    if (!enabled) return;
    
    Event event{
        type,
        name,
        properties,
        QDateTime::currentMSecsSinceEpoch()
    };
    
    queueEvent(event);
    emit eventTracked(event);
}

void Analytics::trackError(const QString& error, const QString& stackTrace) {
    QJsonObject properties;
    properties["error"] = error;
    if (!stackTrace.isEmpty()) {
        properties["stackTrace"] = stackTrace;
    }
    
    trackEvent(EventType::Error, "error", properties);
}

void Analytics::trackPerformance(const QString& operation, qint64 duration) {
    QJsonObject properties;
    properties["operation"] = operation;
    properties["duration"] = duration;
    
    trackEvent(EventType::Performance, "performance", properties);
}

void Analytics::trackFeatureUsage(const QString& feature) {
    QJsonObject properties;
    properties["feature"] = feature;
    
    trackEvent(EventType::FeatureUsage, "feature_usage", properties);
}

void Analytics::startSession() {
    sessionId = QUuid::createUuid().toString();
    sessionStartTime = QDateTime::currentMSecsSinceEpoch();
    
    QJsonObject properties;
    properties["sessionId"] = sessionId;
    
    trackEvent(EventType::AppStart, "session_start", properties);
}

void Analytics::endSession() {
    if (sessionId.isEmpty()) return;
    
    QJsonObject properties;
    properties["sessionId"] = sessionId;
    properties["duration"] = QDateTime::currentMSecsSinceEpoch() - sessionStartTime;
    
    trackEvent(EventType::AppExit, "session_end", properties);
    processEventQueue(); // Flush remaining events
}

void Analytics::setEnabled(bool enabled) {
    this->enabled = enabled;
    if (!enabled) {
        eventQueue.clear();
    }
}

void Analytics::setUserId(const QString& userId) {
    this->userId = userId;
}

void Analytics::setCustomProperty(const QString& key, const QVariant& value) {
    customProperties[key] = QJsonValue::fromVariant(value);
}

void Analytics::enableCrashReporting(bool enable) {
    crashReportingEnabled = enable;
}

void Analytics::reportCrash(const QString& reason, const QString& stackTrace) {
    if (!crashReportingEnabled) return;
    
    QJsonObject properties;
    properties["reason"] = reason;
    properties["stackTrace"] = stackTrace;
    
    trackEvent(EventType::Error, "crash", properties);
    processEventQueue(); // Ensure crash is reported immediately
}

void Analytics::queueEvent(const Event& event) {
    if (eventQueue.size() >= MAX_QUEUE_SIZE) {
        eventQueue.dequeue();
    }
    
    eventQueue.enqueue(event);
    
    if (eventQueue.size() >= MAX_BATCH_SIZE) {
        processEventQueue();
    }
}

void Analytics::processEventQueue() {
    if (eventQueue.isEmpty()) return;
    
    QJsonArray events;
    int batchSize = qMin(eventQueue.size(), MAX_BATCH_SIZE);
    
    for (int i = 0; i < batchSize; ++i) {
        Event event = eventQueue.dequeue();
        
        QJsonObject eventObj;
        eventObj["type"] = static_cast<int>(event.type);
        eventObj["name"] = event.name;
        eventObj["timestamp"] = event.timestamp;
        eventObj["properties"] = event.properties;
        
        // Add base properties
        QJsonObject baseProps = createBaseProperties();
        for (auto it = baseProps.begin(); it != baseProps.end(); ++it) {
            eventObj[it.key()] = it.value();
        }
        
        events.append(eventObj);
    }
    
    QJsonObject payload;
    payload["events"] = events;
    
    QNetworkRequest request(QUrl(API_ENDPOINT));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void Analytics::handleNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
    }
}

QJsonObject Analytics::createBaseProperties() const {
    QJsonObject props;
    
    // App info
    props["appVersion"] = EDITOR_VERSION_STRING;
    props["appName"] = EDITOR_PRODUCT_NAME;
    
    // User info
    if (!userId.isEmpty()) {
        props["userId"] = userId;
    }
    props["sessionId"] = sessionId;
    
    // System info
    props["os"] = getOSInfo();
    props["device"] = getDeviceInfo();
    
    // Screen info
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        props["screenResolution"] = QString("%1x%2")
            .arg(screen->size().width())
            .arg(screen->size().height());
    }
    
    // Custom properties
    for (auto it = customProperties.begin(); it != customProperties.end(); ++it) {
        props[it.key()] = it.value();
    }
    
    return props;
}

QString Analytics::getOSInfo() const {
    return QString("%1 %2")
        .arg(QSysInfo::prettyProductName())
        .arg(QSysInfo::currentCpuArchitecture());
}

QString Analytics::getDeviceInfo() const {
    return QSysInfo::machineHostName();
}

void Analytics::saveEventQueue() {
    QString path = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    
    QFile file(path + "/analytics_queue.json");
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    
    QJsonArray events;
    for (const Event& event : eventQueue) {
        QJsonObject eventObj;
        eventObj["type"] = static_cast<int>(event.type);
        eventObj["name"] = event.name;
        eventObj["timestamp"] = event.timestamp;
        eventObj["properties"] = event.properties;
        events.append(eventObj);
    }
    
    file.write(QJsonDocument(events).toJson());
}

void Analytics::loadEventQueue() {
    QString path = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QFile file(path + "/analytics_queue.json");
    
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return;
    }
    
    QJsonArray events = doc.array();
    for (const QJsonValue& value : events) {
        if (!value.isObject()) continue;
        
        QJsonObject obj = value.toObject();
        Event event{
            static_cast<EventType>(obj["type"].toInt()),
            obj["name"].toString(),
            obj["properties"].toObject(),
            obj["timestamp"].toVariant().toLongLong()
        };
        
        eventQueue.enqueue(event);
    }
}
