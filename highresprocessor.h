#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include <QImage>
#include <memory>
#include <vector>
#include "gpumanager.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
}

class HighResProcessor : public QObject {
    Q_OBJECT

public:
    struct VideoInfo {
        int width;
        int height;
        double fps;
        int64_t bitrate;
        QString codec;
        bool isHDR;
        QString colorSpace;
        QString pixelFormat;
    };

    struct ProcessingOptions {
        bool useGPU;
        bool preserveHDR;
        int quality;
        QString outputCodec;
        QString outputFormat;
        QSize outputSize;
        int outputBitrate;
        bool enableDenoising;
        bool enableSharpening;
        bool enableStabilization;
    };

    static HighResProcessor& instance();

    // Initialization and setup
    bool initialize();
    bool isInitialized() const;
    VideoInfo getVideoInfo(const QString& filePath) const;
    bool setProcessingOptions(const ProcessingOptions& options);

    // Video processing
    bool openVideo(const QString& filePath);
    bool processFrame(AVFrame* frame);
    bool processFrameGPU(AVFrame* frame);
    bool writeFrame(AVFrame* frame);
    bool finishProcessing();

    // HDR processing
    bool processHDRFrame(AVFrame* frame);
    bool convertHDRtoSDR(AVFrame* frame);
    bool applyHDRToneMapping(AVFrame* frame);

    // Frame operations
    bool scaleFrame(AVFrame* src, AVFrame* dst);
    bool denoiseFrame(AVFrame* frame);
    bool sharpenFrame(AVFrame* frame);
    bool stabilizeFrame(AVFrame* frame);

    // Performance monitoring
    float getProcessingProgress() const;
    QString getProcessingStats() const;
    bool cancelProcessing();

signals:
    void processingProgress(float progress);
    void processingStats(const QString& stats);
    void frameProcessed(const QImage& frame);
    void errorOccurred(const QString& error);
    void processingFinished();

private:
    explicit HighResProcessor(QObject* parent = nullptr);
    ~HighResProcessor();

    // Internal helper functions
    bool initializeCodecs();
    bool initializeFilters();
    bool setupScaler();
    bool allocateFrameBuffers();
    void cleanupResources();

    // Frame conversion helpers
    bool convertFrameToRGB(AVFrame* frame, QImage& image);
    bool convertRGBToFrame(const QImage& image, AVFrame* frame);

    // Error handling
    QString getErrorString(int error) const;
    void logError(const QString& error);

    // Member variables
    bool initialized;
    ProcessingOptions options;
    VideoInfo currentVideo;
    QString lastError;

    // FFmpeg contexts
    AVFormatContext* formatContext;
    AVCodecContext* decoderContext;
    AVCodecContext* encoderContext;
    SwsContext* scalerContext;
    AVFilterGraph* filterGraph;
    AVFilterContext* bufferSrcContext;
    AVFilterContext* bufferSinkContext;

    // Frame buffers
    AVFrame* inputFrame;
    AVFrame* processedFrame;
    AVFrame* outputFrame;

    // Processing state
    int64_t totalFrames;
    int64_t processedFrames;
    bool processingCancelled;

    // Performance metrics
    struct ProcessingMetrics {
        double averageProcessingTime;
        double peakMemoryUsage;
        int droppedFrames;
        double fps;
    } metrics;

    // Constants
    static const int MAX_FRAME_SIZE;
    static const int DEFAULT_BUFFER_SIZE;
};
