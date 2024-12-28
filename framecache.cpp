#include "framecache.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

// FrameLoader implementation
FrameLoader::FrameLoader(QObject* parent)
    : QThread(parent)
    , running(true)
{
}

FrameLoader::~FrameLoader() {
    stop();
    wait();
}

void FrameLoader::requestFrame(const QString& filePath, qint64 timestamp) {
    QMutexLocker locker(&mutex);
    requestQueue.enqueue({filePath, timestamp});
    if (!isRunning()) {
        start();
    }
}

void FrameLoader::stop() {
    QMutexLocker locker(&mutex);
    running = false;
    requestQueue.clear();
}

void FrameLoader::run() {
    while (running) {
        LoadRequest request;
        {
            QMutexLocker locker(&mutex);
            if (requestQueue.isEmpty()) {
                // No more requests, exit thread
                return;
            }
            request = requestQueue.dequeue();
        }
        
        QImage frame = loadFrame(request.filePath, request.timestamp);
        if (!frame.isNull()) {
            emit frameLoaded(request.filePath, request.timestamp, frame);
        } else {
            emit frameLoadError(request.filePath, request.timestamp, 
                              "Failed to load frame");
        }
    }
}

QImage FrameLoader::loadFrame(const QString& filePath, qint64 timestamp) {
    // Create temporary file for the frame
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        qDebug() << "Failed to create temporary file for frame extraction";
        return QImage();
    }
    
    // Build FFmpeg command to extract frame
    QStringList arguments;
    arguments << "-ss" << QString::number(timestamp / 1000.0);  // Convert to seconds
    arguments << "-i" << filePath;
    arguments << "-vframes" << "1";
    arguments << "-f" << "image2";
    arguments << "-c:v" << "png";
    arguments << tempFile.fileName();
    
    // Run FFmpeg
    QProcess process;
    process.start("ffmpeg", arguments);
    
    if (!process.waitForFinished()) {
        qDebug() << "Failed to extract frame:" << process.errorString();
        return QImage();
    }
    
    // Load the extracted frame
    QImage frame(tempFile.fileName());
    return frame;
}

// FrameCache implementation
FrameCache::FrameCache(QObject* parent)
    : QObject(parent)
    , frameLoader(std::make_unique<FrameLoader>())
    , maxCacheSize(512)  // Default to 512MB
    , cacheAhead(30)     // Cache 1 second ahead at 30fps
    , cacheBehind(30)    // Cache 1 second behind at 30fps
    , cacheHits(0)
    , cacheMisses(0)
{
    // Set up frame cache
    frameCache.setMaxCost(maxCacheSize * 1024 * 1024);  // Convert MB to bytes
    
    // Connect frame loader signals
    connect(frameLoader.get(), &FrameLoader::frameLoaded,
            this, &FrameCache::handleFrameLoaded);
    connect(frameLoader.get(), &FrameLoader::frameLoadError,
            this, &FrameCache::handleFrameLoadError);
}

FrameCache::~FrameCache() {
    frameLoader->stop();
}

void FrameCache::setMaxCacheSize(int megabytes) {
    maxCacheSize = megabytes;
    frameCache.setMaxCost(megabytes * 1024 * 1024);
}

QImage FrameCache::getFrame(const QString& filePath, qint64 timestamp) {
    CacheKey key{filePath, timestamp};
    
    // Try to get frame from cache
    QImage* frame = frameCache.object(key);
    if (frame) {
        cacheHits++;
        return *frame;
    }
    
    // Frame not in cache
    cacheMisses++;
    
    // Request frame loading
    frameLoader->requestFrame(filePath, timestamp);
    
    // Prefetch nearby frames
    qint64 frameInterval = 1000 / 30;  // Assume 30fps
    for (int i = 1; i <= cacheAhead; i++) {
        prefetchFrame(filePath, timestamp + i * frameInterval);
    }
    for (int i = 1; i <= cacheBehind; i++) {
        prefetchFrame(filePath, timestamp - i * frameInterval);
    }
    
    return QImage();  // Return empty image, frame will be available later
}

void FrameCache::prefetchFrames(const QString& filePath, qint64 startTime, 
                              qint64 endTime) {
    qint64 frameInterval = 1000 / 30;  // Assume 30fps
    for (qint64 time = startTime; time <= endTime; time += frameInterval) {
        prefetchFrame(filePath, time);
    }
}

void FrameCache::clearCache() {
    frameCache.clear();
    resetStatistics();
}

int FrameCache::getCacheSize() const {
    return frameCache.totalCost() / (1024 * 1024);  // Convert bytes to MB
}

void FrameCache::resetStatistics() {
    cacheHits = 0;
    cacheMisses = 0;
}

void FrameCache::handleFrameLoaded(const QString& filePath, qint64 timestamp, 
                                 const QImage& frame) {
    insertFrame(filePath, timestamp, frame);
    emit frameAvailable(filePath, timestamp);
}

void FrameCache::handleFrameLoadError(const QString& filePath, qint64 timestamp, 
                                    const QString& error) {
    QString message = QString("Failed to load frame at %1ms from %2: %3")
        .arg(timestamp)
        .arg(filePath)
        .arg(error);
    emit cacheError(message);
}

void FrameCache::insertFrame(const QString& filePath, qint64 timestamp, 
                           const QImage& frame) {
    CacheKey key{filePath, timestamp};
    
    // Calculate frame size in bytes (approximate)
    int cost = frame.sizeInBytes();
    
    // Store frame in cache
    frameCache.insert(key, new QImage(frame), cost);
}

void FrameCache::prefetchFrame(const QString& filePath, qint64 timestamp) {
    CacheKey key{filePath, timestamp};
    
    // Only prefetch if frame is not already in cache
    if (!frameCache.contains(key)) {
        frameLoader->requestFrame(filePath, timestamp);
    }
}
