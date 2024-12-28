#include "updatechecker.h"
#include "version.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>

QString UpdateChecker::Version::toString() const {
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

bool UpdateChecker::Version::operator>(const Version& other) const {
    if (major != other.major) return major > other.major;
    if (minor != other.minor) return minor > other.minor;
    return patch > other.patch;
}

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , networkManager(std::make_unique<QNetworkAccessManager>())
    , currentDownload(nullptr)
    , silent(false)
{
    updateUrl = EDITOR_UPDATE_URL;
    
    connect(&autoCheckTimer, &QTimer::timeout,
            this, &UpdateChecker::handleAutoCheckTimer);
    
    connect(networkManager.get(), &QNetworkAccessManager::finished,
            this, &UpdateChecker::handleNetworkReply);
}

UpdateChecker::~UpdateChecker() {
    if (currentDownload) {
        currentDownload->abort();
    }
}

void UpdateChecker::checkForUpdates(bool silent) {
    this->silent = silent;
    
    QNetworkRequest request(QUrl(updateUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     QString("%1/%2").arg(EDITOR_PRODUCT_NAME)
                                   .arg(EDITOR_VERSION_STRING));
    
    networkManager->get(request);
}

void UpdateChecker::setAutomaticChecks(bool enable, int intervalHours) {
    if (enable) {
        autoCheckTimer.start(intervalHours * 60 * 60 * 1000);
    } else {
        autoCheckTimer.stop();
    }
}

UpdateChecker::Version UpdateChecker::getCurrentVersion() const {
    return {EDITOR_VERSION_MAJOR, EDITOR_VERSION_MINOR, EDITOR_VERSION_PATCH};
}

UpdateChecker::Version UpdateChecker::getLatestVersion() const {
    return latestUpdate.version;
}

bool UpdateChecker::isUpdateAvailable() const {
    return latestUpdate.version > getCurrentVersion();
}

void UpdateChecker::setUpdateUrl(const QString& url) {
    updateUrl = url;
}

void UpdateChecker::setProxySettings(const QNetworkProxy& proxy) {
    networkManager->setProxy(proxy);
}

void UpdateChecker::downloadUpdate() {
    if (!isUpdateAvailable() || currentDownload) {
        return;
    }
    
    startDownload(latestUpdate.downloadUrl);
}

void UpdateChecker::cancelDownload() {
    if (currentDownload) {
        currentDownload->abort();
        currentDownload = nullptr;
    }
}

void UpdateChecker::handleNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();
    
    if (reply == currentDownload) {
        handleDownloadComplete(reply);
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        if (!silent) {
            emit checkFailed(reply->errorString());
        }
        return;
    }
    
    processUpdateResponse(reply->readAll());
}

void UpdateChecker::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateChecker::handleAutoCheckTimer() {
    checkForUpdates(true);
}

UpdateChecker::Version UpdateChecker::parseVersionString(const QString& version) const {
    QStringList parts = version.split('.');
    if (parts.size() != 3) {
        return {0, 0, 0};
    }
    
    return {
        parts[0].toInt(),
        parts[1].toInt(),
        parts[2].toInt()
    };
}

void UpdateChecker::processUpdateResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        if (!silent) {
            emit checkFailed("Invalid update response format");
        }
        return;
    }
    
    QJsonObject obj = doc.object();
    UpdateInfo info;
    info.version = parseVersionString(obj["version"].toString());
    info.downloadUrl = obj["download_url"].toString();
    info.releaseNotes = obj["release_notes"].toString();
    info.isRequired = obj["required"].toBool();
    
    latestUpdate = info;
    
    if (isUpdateAvailable()) {
        emit updateAvailable(info);
    } else if (!silent) {
        emit noUpdateAvailable();
    }
}

QString UpdateChecker::getDownloadPath() const {
    QString downloadPath = QStandardPaths::writableLocation(
        QStandardPaths::DownloadLocation);
    return QDir(downloadPath).filePath(
        QString("%1-%2.exe").arg(EDITOR_PRODUCT_NAME)
                           .arg(latestUpdate.version.toString()));
}

void UpdateChecker::startDownload(const QString& url) {
    QNetworkRequest request(url);
    currentDownload = networkManager->get(request);
    
    connect(currentDownload, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::handleDownloadProgress);
}

void UpdateChecker::handleDownloadComplete(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFailed(reply->errorString());
        currentDownload = nullptr;
        return;
    }
    
    QString filePath = getDownloadPath();
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadFailed("Failed to create download file");
        currentDownload = nullptr;
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    
    emit downloadComplete(filePath);
    currentDownload = nullptr;
}
