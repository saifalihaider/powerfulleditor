#include <gtest/gtest.h>
#include <QTemporaryFile>
#include <QDir>
#include "../src/videoexporter.h"
#include "../src/proxymanager.h"
#include "../src/framecache.h"

class VideoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        tempDir.reset(new QTemporaryDir());
        ASSERT_TRUE(tempDir->isValid());
        
        // Initialize components
        exporter = std::make_unique<VideoExporter>();
        proxyManager = std::make_unique<ProxyManager>(tempDir->path());
        frameCache = std::make_unique<FrameCache>();
    }
    
    void TearDown() override {
        exporter.reset();
        proxyManager.reset();
        frameCache.reset();
        tempDir.reset();
    }
    
    // Helper functions
    QString createTestVideo(const QString& name, int duration = 10) {
        QString filePath = tempDir->filePath(name);
        // Create a test video file using FFmpeg
        QString command = QString("ffmpeg -f lavfi -i color=c=red:s=1280x720:d=%1 -c:v libx264 %2")
                         .arg(duration).arg(filePath);
        system(qPrintable(command));
        return filePath;
    }
    
    std::unique_ptr<QTemporaryDir> tempDir;
    std::unique_ptr<VideoExporter> exporter;
    std::unique_ptr<ProxyManager> proxyManager;
    std::unique_ptr<FrameCache> frameCache;
};

// Video Export Tests
TEST_F(VideoTest, TestBasicExport) {
    QString inputPath = createTestVideo("input.mp4");
    QString outputPath = tempDir->filePath("output.mp4");
    
    ExportSettings settings;
    settings.setOutputPath(outputPath);
    settings.setVideoCodec("libx264");
    settings.setResolution(1280, 720);
    
    ASSERT_TRUE(exporter->startExport(inputPath, settings));
    ASSERT_TRUE(QFile::exists(outputPath));
}

TEST_F(VideoTest, TestInvalidExport) {
    QString outputPath = tempDir->filePath("output.mp4");
    ExportSettings settings;
    settings.setOutputPath(outputPath);
    
    ASSERT_FALSE(exporter->startExport("nonexistent.mp4", settings));
}

// Proxy Manager Tests
TEST_F(VideoTest, TestProxyCreation) {
    QString inputPath = createTestVideo("input.mp4");
    
    ASSERT_TRUE(proxyManager->createProxy(inputPath));
    ASSERT_TRUE(proxyManager->hasProxy(inputPath));
    
    QString proxyPath = proxyManager->getProxyPath(inputPath);
    ASSERT_FALSE(proxyPath.isEmpty());
    ASSERT_TRUE(QFile::exists(proxyPath));
}

TEST_F(VideoTest, TestProxyDeletion) {
    QString inputPath = createTestVideo("input.mp4");
    ASSERT_TRUE(proxyManager->createProxy(inputPath));
    
    ASSERT_TRUE(proxyManager->removeProxy(inputPath));
    ASSERT_FALSE(proxyManager->hasProxy(inputPath));
}

// Frame Cache Tests
TEST_F(VideoTest, TestFrameCaching) {
    QString inputPath = createTestVideo("input.mp4");
    
    // Test frame loading
    QImage frame = frameCache->getFrame(inputPath, 0);
    ASSERT_FALSE(frame.isNull());
    
    // Test cache hit
    QImage cachedFrame = frameCache->getFrame(inputPath, 0);
    ASSERT_FALSE(cachedFrame.isNull());
    ASSERT_GT(frameCache->getCacheHits(), 0);
}

TEST_F(VideoTest, TestCacheSize) {
    frameCache->setMaxCacheSize(100); // 100MB
    ASSERT_EQ(frameCache->getMaxCacheSize(), 100);
    
    QString inputPath = createTestVideo("input.mp4");
    
    // Load multiple frames
    for (int i = 0; i < 30; i++) {
        frameCache->getFrame(inputPath, i * 1000/30); // Assuming 30fps
    }
    
    // Check cache size
    ASSERT_LE(frameCache->getCacheSize(), 100);
}

// Performance Tests
TEST_F(VideoTest, TestExportPerformance) {
    QString inputPath = createTestVideo("input.mp4", 30); // 30-second video
    QString outputPath = tempDir->filePath("output.mp4");
    
    ExportSettings settings;
    settings.setOutputPath(outputPath);
    settings.setVideoCodec("libx264");
    settings.setResolution(1280, 720);
    
    QElapsedTimer timer;
    timer.start();
    
    ASSERT_TRUE(exporter->startExport(inputPath, settings));
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Export time for 30s video:" << elapsed << "ms";
    
    // Export should take reasonable time
    ASSERT_LT(elapsed, 60000); // Should take less than 60 seconds
}

TEST_F(VideoTest, TestProxyPerformance) {
    QString inputPath = createTestVideo("input.mp4", 30);
    
    QElapsedTimer timer;
    timer.start();
    
    ASSERT_TRUE(proxyManager->createProxy(inputPath));
    
    qint64 elapsed = timer.elapsed();
    qDebug() << "Proxy creation time for 30s video:" << elapsed << "ms";
    
    // Proxy creation should be faster than real-time
    ASSERT_LT(elapsed, 30000); // Should take less than 30 seconds
}

// Error Handling Tests
TEST_F(VideoTest, TestExportErrorHandling) {
    ExportSettings settings;
    settings.setOutputPath("/invalid/path/output.mp4");
    
    ASSERT_FALSE(exporter->startExport("input.mp4", settings));
    ASSERT_FALSE(exporter->getLastError().isEmpty());
}

TEST_F(VideoTest, TestProxyErrorHandling) {
    ASSERT_FALSE(proxyManager->createProxy("nonexistent.mp4"));
    ASSERT_FALSE(proxyManager->getLastError().isEmpty());
}
