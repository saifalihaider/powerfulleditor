#include "proxymanager.h"
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QProcess>
#include <QDebug>

ProxyManager::ProxyManager(const QString& cacheDir, QObject* parent)
    : QObject(parent)
    , cacheDir(cacheDir)
    , proxyProcess(std::make_unique<QProcess>())
    , progress(0.0)
{
    // Set up process
    proxyProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    // Connect signals
    connect(proxyProcess.get(), &QProcess::readyRead,
            this, &ProxyManager::handleProcessOutput);
    connect(proxyProcess.get(), &QProcess::errorOccurred,
            this, &ProxyManager::handleProcessError);
    connect(proxyProcess.get(), 
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &ProxyManager::handleProcessFinished);
    
    // Create cache directory if it doesn't exist
    ensureCacheDirectory();
}

ProxyManager::~ProxyManager() {
    if (proxyProcess && proxyProcess->state() != QProcess::NotRunning) {
        proxyProcess->kill();
        proxyProcess->waitForFinished();
    }
}

bool ProxyManager::createProxy(const QString& sourceFile) {
    if (isGeneratingProxy()) {
        reportError("Proxy generation already in progress");
        return false;
    }
    
    // Check source file
    QFileInfo sourceInfo(sourceFile);
    if (!sourceInfo.exists() || !sourceInfo.isReadable()) {
        reportError("Source file does not exist or is not readable");
        return false;
    }
    
    // Ensure cache directory exists
    if (!ensureCacheDirectory()) {
        return false;
    }
    
    // Generate proxy path
    QString proxyPath = generateProxyPath(sourceFile);
    proxyFiles[sourceFile] = proxyPath;
    
    // Build FFmpeg command
    QStringList arguments = buildFFmpegCommand(sourceFile, proxyPath);
    
    // Start proxy generation
    currentSourceFile = sourceFile;
    progress = 0.0;
    proxyProcess->start("ffmpeg", arguments);
    
    if (!proxyProcess->waitForStarted()) {
        reportError("Failed to start FFmpeg process");
        return false;
    }
    
    emit proxyGenerationStarted(sourceFile);
    return true;
}

QString ProxyManager::getProxyPath(const QString& sourceFile) const {
    return proxyFiles.value(sourceFile);
}

bool ProxyManager::hasProxy(const QString& sourceFile) const {
    QString proxyPath = proxyFiles.value(sourceFile);
    if (proxyPath.isEmpty()) return false;
    
    QFileInfo info(proxyPath);
    return info.exists() && info.isReadable();
}

void ProxyManager::removeProxy(const QString& sourceFile) {
    QString proxyPath = proxyFiles.take(sourceFile);
    if (!proxyPath.isEmpty()) {
        QFile::remove(proxyPath);
    }
}

void ProxyManager::clearAllProxies() {
    // Remove all proxy files
    for (const QString& proxyPath : proxyFiles.values()) {
        QFile::remove(proxyPath);
    }
    proxyFiles.clear();
    
    emit cacheCleared();
}

void ProxyManager::setCacheDirectory(const QString& dir) {
    if (cacheDir != dir) {
        // Clear existing cache
        clearAllProxies();
        
        cacheDir = dir;
        ensureCacheDirectory();
    }
}

qint64 ProxyManager::getCacheSize() const {
    qint64 totalSize = 0;
    QDir dir(cacheDir);
    
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        totalSize += info.size();
    }
    
    return totalSize;
}

void ProxyManager::clearCache() {
    clearAllProxies();
    
    // Remove cache directory
    QDir dir(cacheDir);
    dir.removeRecursively();
    
    // Recreate empty cache directory
    ensureCacheDirectory();
}

bool ProxyManager::isGeneratingProxy() const {
    return proxyProcess && proxyProcess->state() != QProcess::NotRunning;
}

QString ProxyManager::generateProxyPath(const QString& sourceFile) const {
    // Generate unique filename based on source file path
    QByteArray hash = QCryptographicHash::hash(
        sourceFile.toUtf8(), QCryptographicHash::Md5).toHex();
    
    return QDir(cacheDir).filePath(QString("%1_proxy.mp4").arg(QString(hash)));
}

QStringList ProxyManager::buildFFmpegCommand(const QString& sourceFile,
                                           const QString& proxyFile) const {
    QStringList arguments;
    
    // Add hardware acceleration if available
    auto hwaccel = detectHardwareAcceleration();
    if (hwaccel) {
        arguments << "-hwaccel" << *hwaccel;
    }
    
    // Input file
    arguments << "-i" << sourceFile;
    
    // Video settings
    arguments << "-c:v" << "libx264"
             << "-preset" << proxySettings.preset
             << "-b:v" << QString("%1k").arg(proxySettings.bitrate)
             << "-r" << QString::number(proxySettings.frameRate)
             << "-s" << QString("%1x%2")
                .arg(proxySettings.resolution.width())
                .arg(proxySettings.resolution.height());
    
    // Audio settings (low quality, we don't need high quality for proxy)
    arguments << "-c:a" << "aac"
             << "-b:a" << "64k"
             << "-ac" << "2";
    
    // Progress reporting
    arguments << "-progress" << "-" << "-nostats";
    
    // Output file (overwrite if exists)
    arguments << "-y" << proxyFile;
    
    return arguments;
}

bool ProxyManager::ensureCacheDirectory() const {
    QDir dir(cacheDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        qDebug() << "Failed to create cache directory:" << cacheDir;
        return false;
    }
    return true;
}

void ProxyManager::handleProcessOutput() {
    QString output = QString::fromUtf8(proxyProcess->readAll());
    
    // Parse progress information
    static QRegularExpression timeRegex("time=([\\d:.]+)");
    QRegularExpressionMatch match = timeRegex.match(output);
    
    if (match.hasMatch()) {
        QString timeStr = match.captured(1);
        double currentTime = parseTime(timeStr);
        double totalDuration = parseDuration(output);
        
        if (totalDuration > 0) {
            progress = (currentTime / totalDuration) * 100.0;
            emit proxyGenerationProgress(currentSourceFile, progress);
        }
    }
}

void ProxyManager::handleProcessError(QProcess::ProcessError error) {
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start FFmpeg process";
            break;
        case QProcess::Crashed:
            errorMessage = "FFmpeg process crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "FFmpeg process timed out";
            break;
        case QProcess::WriteError:
            errorMessage = "Failed to write to FFmpeg process";
            break;
        case QProcess::ReadError:
            errorMessage = "Failed to read from FFmpeg process";
            break;
        default:
            errorMessage = "Unknown FFmpeg process error";
    }
    
    reportError(errorMessage);
}

void ProxyManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        progress = 100.0;
        emit proxyGenerationProgress(currentSourceFile, progress);
        emit proxyGenerationFinished(currentSourceFile, true);
    } else {
        QString error = QString::fromUtf8(proxyProcess->readAllStandardError());
        reportError("Proxy generation failed: " + error);
        emit proxyGenerationFinished(currentSourceFile, false);
        
        // Clean up failed proxy file
        removeProxy(currentSourceFile);
    }
    
    currentSourceFile.clear();
}

void ProxyManager::reportError(const QString& error) {
    lastError = error;
    qDebug() << "Proxy error:" << error;
    
    if (!currentSourceFile.isEmpty()) {
        emit proxyGenerationError(currentSourceFile, error);
    }
}

double ProxyManager::parseDuration(const QString& output) const {
    QRegularExpression durationRegex("Duration: ([\\d:.]+)");
    QRegularExpressionMatch match = durationRegex.match(output);
    
    if (match.hasMatch()) {
        return parseTime(match.captured(1));
    }
    
    return 0.0;
}

double ProxyManager::parseTime(const QString& timeStr) const {
    QStringList parts = timeStr.split(":");
    if (parts.size() == 3) {
        double hours = parts[0].toDouble();
        double minutes = parts[1].toDouble();
        double seconds = parts[2].toDouble();
        
        return hours * 3600 + minutes * 60 + seconds;
    }
    
    return 0.0;
}

std::optional<QString> ProxyManager::detectHardwareAcceleration() const {
    // Check for NVIDIA GPU
    QProcess nvidia;
    nvidia.start("nvidia-smi");
    if (nvidia.waitForFinished() && nvidia.exitCode() == 0) {
        return "cuda";
    }
    
    // Check for Intel QuickSync
    QProcess intel;
    intel.start("ffmpeg", {"-hwaccels"});
    if (intel.waitForFinished() && 
        QString(intel.readAll()).contains("qsv", Qt::CaseInsensitive)) {
        return "qsv";
    }
    
    // No hardware acceleration detected
    return std::nullopt;
}
