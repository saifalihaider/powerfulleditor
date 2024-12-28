#include "bugreporter.h"
#include "version.h"
#include <QSysInfo>
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>

const QString BugReporter::DEFAULT_API_ENDPOINT = "https://bugs.example.com/v1/reports";
const int BugReporter::MAX_SCREENSHOT_SIZE = 1920;
const int BugReporter::MAX_LOG_SIZE = 10 * 1024 * 1024; // 10MB
const int BugReporter::MAX_LOG_FILES = 5;

BugReporter& BugReporter::instance() {
    static BugReporter instance;
    return instance;
}

BugReporter::BugReporter(QObject* parent)
    : QObject(parent)
    , networkManager(std::make_unique<QNetworkAccessManager>())
    , apiEndpoint(DEFAULT_API_ENDPOINT)
{
    connect(networkManager.get(), &QNetworkAccessManager::finished,
            this, &BugReporter::handleNetworkReply);
}

BugReporter::~BugReporter() = default;

void BugReporter::submitReport(const BugReport& report) {
    if (!validateReport(report)) {
        emit errorOccurred(lastError);
        return;
    }
    
    QJsonObject reportJson = createReportJson(report);
    sendReport(reportJson, report.screenshot);
}

void BugReporter::submitCrashReport(const QString& crashReason,
                                  const QString& stackTrace) {
    BugReport report;
    report.title = "Crash Report: " + crashReason;
    report.description = crashReason;
    report.stackTrace = stackTrace;
    report.systemInfo = getSystemInfo();
    report.logContent = getLogContent();
    
    submitReport(report);
}

void BugReporter::setApiKey(const QString& key) {
    apiKey = key;
}

void BugReporter::setEndpoint(const QString& url) {
    apiEndpoint = url;
}

void BugReporter::setUserEmail(const QString& email) {
    userEmail = email;
}

QString BugReporter::getSystemInfo() const {
    QString info;
    QTextStream stream(&info);
    
    // App information
    stream << "Application: " << EDITOR_PRODUCT_NAME << "\n";
    stream << "Version: " << EDITOR_VERSION_STRING << "\n\n";
    
    // System information
    stream << "OS: " << QSysInfo::prettyProductName() << "\n";
    stream << "Kernel: " << QSysInfo::kernelVersion() << "\n";
    stream << "Architecture: " << QSysInfo::currentCpuArchitecture() << "\n\n";
    
    // Hardware information
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        stream << "Screen Resolution: "
               << screen->size().width() << "x"
               << screen->size().height() << "\n";
    }
    
    // Memory information
    #ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        stream << "Total Physical Memory: "
               << memInfo.ullTotalPhys / (1024*1024) << " MB\n";
        stream << "Available Physical Memory: "
               << memInfo.ullAvailPhys / (1024*1024) << " MB\n";
    }
    #endif
    
    return info;
}

QString BugReporter::getLogContent() const {
    QString logPath = getLogFilePath();
    QFile logFile(logPath);
    
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    return QString::fromUtf8(logFile.readAll());
}

QImage BugReporter::captureScreenshot() const {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return QImage();
    
    QImage screenshot = screen->grabWindow(0).toImage();
    
    // Resize if too large
    if (screenshot.width() > MAX_SCREENSHOT_SIZE ||
        screenshot.height() > MAX_SCREENSHOT_SIZE) {
        screenshot = screenshot.scaled(MAX_SCREENSHOT_SIZE, MAX_SCREENSHOT_SIZE,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    }
    
    return screenshot;
}

QString BugReporter::getLastError() const {
    return lastError;
}

void BugReporter::clearLastError() {
    lastError.clear();
}

void BugReporter::sendReport(const QJsonObject& reportData,
                           const QImage& screenshot) {
    QString reportId = generateReportId();
    QJsonObject finalReport = reportData;
    finalReport["reportId"] = reportId;
    
    // Save screenshot if provided
    if (!screenshot.isNull()) {
        if (saveScreenshot(screenshot, reportId)) {
            finalReport["hasScreenshot"] = true;
        }
    }
    
    QNetworkRequest request(QUrl(apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!apiKey.isEmpty()) {
        request.setRawHeader("X-API-Key", apiKey.toUtf8());
    }
    
    networkManager->post(request, QJsonDocument(finalReport).toJson());
}

void BugReporter::handleNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        lastError = reply->errorString();
        emit errorOccurred(lastError);
        emit reportSubmitted(false, QString());
        return;
    }
    
    QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
    QString reportId = response.object()["reportId"].toString();
    
    emit reportSubmitted(true, reportId);
}

QJsonObject BugReporter::createReportJson(const BugReport& report) const {
    QJsonObject json;
    
    json["title"] = report.title;
    json["description"] = report.description;
    json["systemInfo"] = report.systemInfo;
    json["logContent"] = report.logContent;
    json["stackTrace"] = report.stackTrace;
    json["userEmail"] = report.userEmail.isEmpty() ? userEmail : report.userEmail;
    json["reproSteps"] = report.reproSteps;
    json["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json["metadata"] = report.metadata;
    
    return json;
}

bool BugReporter::validateReport(const BugReport& report) const {
    if (report.title.isEmpty()) {
        lastError = "Report title is required";
        return false;
    }
    
    if (report.description.isEmpty()) {
        lastError = "Report description is required";
        return false;
    }
    
    return true;
}

QString BugReporter::generateReportId() const {
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString random = QString::number(qrand());
    
    QByteArray data = (timestamp + random).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
    
    return hash.toHex().left(8);
}

bool BugReporter::saveScreenshot(const QImage& screenshot,
                               const QString& reportId) const {
    QString path = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    path = QDir(path).filePath("bug_reports");
    QDir().mkpath(path);
    
    QString filename = QString("screenshot_%1.png").arg(reportId);
    return screenshot.save(QDir(path).filePath(filename));
}

QString BugReporter::getLogFilePath() const {
    QString path = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    return QDir(path).filePath("editor.log");
}

void BugReporter::rotateLogs() {
    QString basePath = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QString logPath = QDir(basePath).filePath("editor.log");
    
    QFileInfo logFile(logPath);
    if (!logFile.exists() || logFile.size() < MAX_LOG_SIZE) {
        return;
    }
    
    // Rotate existing log files
    for (int i = MAX_LOG_FILES - 1; i >= 0; --i) {
        QString oldName = QString("%1.%2").arg(logPath).arg(i);
        QString newName = QString("%1.%2").arg(logPath).arg(i + 1);
        
        if (QFile::exists(newName)) {
            QFile::remove(newName);
        }
        if (QFile::exists(oldName)) {
            QFile::rename(oldName, newName);
        }
    }
    
    // Rename current log file
    QFile::rename(logPath, logPath + ".0");
}
