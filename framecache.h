#pragma once

#include <QObject>
#include <QCache>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QQueue>
#include <memory>

struct CacheKey {
    QString filePath;
    qint64 timestamp;
    
    bool operator==(const CacheKey& other) const {
        return filePath == other.filePath && timestamp == other.timestamp;
    }
};

// Hash function for CacheKey
inline uint qHash(const CacheKey& key) {
    return qHash(key.filePath) ^ qHash(key.timestamp);
}

class FrameLoader : public QThread {
    Q_OBJECT
    
public:
    explicit FrameLoader(QObject* parent = nullptr);
    ~FrameLoader();
    
    void requestFrame(const QString& filePath, qint64 timestamp);
    void stop();

signals:
    void frameLoaded(const QString& filePath, qint64 timestamp, const QImage& frame);
    void frameLoadError(const QString& filePath, qint64 timestamp, const QString& error);

protected:
    void run() override;

private:
    struct LoadRequest {
        QString filePath;
        qint64 timestamp;
    };
    
    QQueue<LoadRequest> requestQueue;
    QMutex mutex;
    bool running;
    
    QImage loadFrame(const QString& filePath, qint64 timestamp);
};

class FrameCache : public QObject {
    Q_OBJECT

public:
    explicit FrameCache(QObject* parent = nullptr);
    ~FrameCache();
    
    // Cache settings
    void setMaxCacheSize(int megabytes);
    int getMaxCacheSize() const { return maxCacheSize; }
    
    void setCacheAhead(int frames) { cacheAhead = frames; }
    int getCacheAhead() const { return cacheAhead; }
    
    void setCacheBehind(int frames) { cacheBehind = frames; }
    int getCacheBehind() const { return cacheBehind; }
    
    // Frame access
    QImage getFrame(const QString& filePath, qint64 timestamp);
    void prefetchFrames(const QString& filePath, qint64 startTime, qint64 endTime);
    void clearCache();
    
    // Cache statistics
    int getCacheSize() const;
    int getCacheHits() const { return cacheHits; }
    int getCacheMisses() const { return cacheMisses; }
    void resetStatistics();

signals:
    void frameAvailable(const QString& filePath, qint64 timestamp);
    void cacheError(const QString& error);

private slots:
    void handleFrameLoaded(const QString& filePath, qint64 timestamp, const QImage& frame);
    void handleFrameLoadError(const QString& filePath, qint64 timestamp, const QString& error);

private:
    QCache<CacheKey, QImage> frameCache;
    std::unique_ptr<FrameLoader> frameLoader;
    int maxCacheSize;
    int cacheAhead;
    int cacheBehind;
    int cacheHits;
    int cacheMisses;
    
    // Helper functions
    void insertFrame(const QString& filePath, qint64 timestamp, const QImage& frame);
    void prefetchFrame(const QString& filePath, qint64 timestamp);
};
