#include "highresprocessor.h"
#include <QDebug>
#include <QThread>
#include <QImage>
#include <QDateTime>

const int HighResProcessor::MAX_FRAME_SIZE = 8192; // Support up to 8K
const int HighResProcessor::DEFAULT_BUFFER_SIZE = 32 * 1024 * 1024; // 32MB

HighResProcessor& HighResProcessor::instance() {
    static HighResProcessor instance;
    return instance;
}

HighResProcessor::HighResProcessor(QObject* parent)
    : QObject(parent)
    , initialized(false)
    , formatContext(nullptr)
    , decoderContext(nullptr)
    , encoderContext(nullptr)
    , scalerContext(nullptr)
    , filterGraph(nullptr)
    , bufferSrcContext(nullptr)
    , bufferSinkContext(nullptr)
    , inputFrame(nullptr)
    , processedFrame(nullptr)
    , outputFrame(nullptr)
    , totalFrames(0)
    , processedFrames(0)
    , processingCancelled(false)
{
    // Initialize metrics
    metrics = {0.0, 0.0, 0, 0.0};
}

HighResProcessor::~HighResProcessor() {
    cleanupResources();
}

bool HighResProcessor::initialize() {
    if (initialized) {
        return true;
    }

    // Initialize FFmpeg
    if (!initializeCodecs() || !initializeFilters()) {
        return false;
    }

    // Initialize GPU if available
    if (GPUManager::instance().initialize()) {
        qDebug() << "GPU acceleration enabled";
    } else {
        qDebug() << "GPU acceleration not available";
    }

    initialized = true;
    return true;
}

bool HighResProcessor::initializeCodecs() {
    // Allocate frame buffers
    inputFrame = av_frame_alloc();
    processedFrame = av_frame_alloc();
    outputFrame = av_frame_alloc();

    if (!inputFrame || !processedFrame || !outputFrame) {
        logError("Failed to allocate frame buffers");
        return false;
    }

    return true;
}

bool HighResProcessor::initializeFilters() {
    filterGraph = avfilter_graph_alloc();
    if (!filterGraph) {
        logError("Failed to create filter graph");
        return false;
    }

    return true;
}

bool HighResProcessor::openVideo(const QString& filePath) {
    if (!initialized) {
        logError("Processor not initialized");
        return false;
    }

    // Open input file
    int ret = avformat_open_input(&formatContext, filePath.toUtf8().constData(),
                                nullptr, nullptr);
    if (ret < 0) {
        logError("Could not open input file: " + getErrorString(ret));
        return false;
    }

    // Get stream information
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        logError("Could not find stream info: " + getErrorString(ret));
        return false;
    }

    // Find video stream
    int videoStream = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        logError("Could not find video stream");
        return false;
    }

    // Get codec parameters
    AVCodecParameters* codecParams = formatContext->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        logError("Unsupported codec");
        return false;
    }

    // Allocate codec context
    decoderContext = avcodec_alloc_context3(codec);
    if (!decoderContext) {
        logError("Could not allocate decoder context");
        return false;
    }

    // Fill codec context parameters
    ret = avcodec_parameters_to_context(decoderContext, codecParams);
    if (ret < 0) {
        logError("Could not copy codec params: " + getErrorString(ret));
        return false;
    }

    // Initialize decoder
    ret = avcodec_open2(decoderContext, codec, nullptr);
    if (ret < 0) {
        logError("Could not open codec: " + getErrorString(ret));
        return false;
    }

    // Set up video info
    currentVideo.width = codecParams->width;
    currentVideo.height = codecParams->height;
    currentVideo.fps = av_q2d(formatContext->streams[videoStream]->r_frame_rate);
    currentVideo.bitrate = codecParams->bit_rate;
    currentVideo.codec = codec->name;
    currentVideo.isHDR = codecParams->color_trc == AVCOL_TRC_SMPTE2084;
    currentVideo.pixelFormat = av_get_pix_fmt_name((AVPixelFormat)codecParams->format);

    // Calculate total frames
    totalFrames = formatContext->streams[videoStream]->nb_frames;
    if (totalFrames <= 0) {
        // Estimate total frames if not available
        totalFrames = (int64_t)(currentVideo.fps *
                               av_q2d(formatContext->streams[videoStream]->duration));
    }

    return setupScaler();
}

bool HighResProcessor::setupScaler() {
    if (!decoderContext) {
        return false;
    }

    // Set up scaler context for format conversion
    scalerContext = sws_getContext(
        decoderContext->width,
        decoderContext->height,
        decoderContext->pix_fmt,
        options.outputSize.width(),
        options.outputSize.height(),
        AV_PIX_FMT_RGB24,
        SWS_BICUBIC,
        nullptr,
        nullptr,
        nullptr
    );

    if (!scalerContext) {
        logError("Could not initialize scaler");
        return false;
    }

    return true;
}

bool HighResProcessor::processFrame(AVFrame* frame) {
    if (!frame || processingCancelled) {
        return false;
    }

    QDateTime startTime = QDateTime::currentDateTime();

    bool success = false;
    if (options.useGPU && GPUManager::instance().isInitialized()) {
        success = processFrameGPU(frame);
    } else {
        // CPU processing
        if (options.enableDenoising) {
            success = denoiseFrame(frame);
        }
        if (success && options.enableSharpening) {
            success = sharpenFrame(frame);
        }
        if (success && options.enableStabilization) {
            success = stabilizeFrame(frame);
        }
    }

    // Update metrics
    if (success) {
        processedFrames++;
        qint64 processingTime = startTime.msecsTo(QDateTime::currentDateTime());
        metrics.averageProcessingTime = (metrics.averageProcessingTime *
                                       (processedFrames - 1) + processingTime) /
                                      processedFrames;
        metrics.fps = 1000.0 / metrics.averageProcessingTime;

        // Emit progress
        float progress = float(processedFrames) / float(totalFrames);
        emit processingProgress(progress);

        // Emit processing stats
        emit processingStats(getProcessingStats());
    } else {
        metrics.droppedFrames++;
    }

    return success;
}

bool HighResProcessor::processFrameGPU(AVFrame* frame) {
    if (!frame) {
        return false;
    }

    // Calculate frame size
    int frameSize = frame->width * frame->height * 3; // Assuming RGB24

    // Process frame using GPU
    return GPUManager::instance().processFrame(
        frame->data[0],
        processedFrame->data[0],
        frame->width,
        frame->height,
        3
    );
}

bool HighResProcessor::processHDRFrame(AVFrame* frame) {
    if (!frame || !currentVideo.isHDR) {
        return false;
    }

    if (options.preserveHDR) {
        return applyHDRToneMapping(frame);
    } else {
        return convertHDRtoSDR(frame);
    }
}

bool HighResProcessor::convertHDRtoSDR(AVFrame* frame) {
    // Implement HDR to SDR conversion
    // This is a placeholder for actual HDR conversion
    return true;
}

bool HighResProcessor::applyHDRToneMapping(AVFrame* frame) {
    // Implement HDR tone mapping
    // This is a placeholder for actual tone mapping
    return true;
}

bool HighResProcessor::denoiseFrame(AVFrame* frame) {
    // Implement denoising
    // This is a placeholder for actual denoising
    return true;
}

bool HighResProcessor::sharpenFrame(AVFrame* frame) {
    // Implement sharpening
    // This is a placeholder for actual sharpening
    return true;
}

bool HighResProcessor::stabilizeFrame(AVFrame* frame) {
    // Implement stabilization
    // This is a placeholder for actual stabilization
    return true;
}

bool HighResProcessor::writeFrame(AVFrame* frame) {
    if (!frame || !encoderContext) {
        return false;
    }

    int ret = avcodec_send_frame(encoderContext, frame);
    if (ret < 0) {
        logError("Error sending frame for encoding: " + getErrorString(ret));
        return false;
    }

    while (ret >= 0) {
        AVPacket* pkt = av_packet_alloc();
        ret = avcodec_receive_packet(encoderContext, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&pkt);
            break;
        } else if (ret < 0) {
            av_packet_free(&pkt);
            logError("Error encoding frame: " + getErrorString(ret));
            return false;
        }

        // Write packet
        ret = av_interleaved_write_frame(formatContext, pkt);
        av_packet_free(&pkt);
        if (ret < 0) {
            logError("Error writing frame: " + getErrorString(ret));
            return false;
        }
    }

    return true;
}

bool HighResProcessor::finishProcessing() {
    if (!initialized) {
        return false;
    }

    // Flush encoder
    writeFrame(nullptr);

    // Write trailer
    if (formatContext) {
        av_write_trailer(formatContext);
    }

    cleanupResources();
    emit processingFinished();

    return true;
}

void HighResProcessor::cleanupResources() {
    if (scalerContext) {
        sws_freeContext(scalerContext);
        scalerContext = nullptr;
    }

    if (inputFrame) {
        av_frame_free(&inputFrame);
    }
    if (processedFrame) {
        av_frame_free(&processedFrame);
    }
    if (outputFrame) {
        av_frame_free(&outputFrame);
    }

    if (decoderContext) {
        avcodec_free_context(&decoderContext);
    }
    if (encoderContext) {
        avcodec_free_context(&encoderContext);
    }

    if (formatContext) {
        avformat_close_input(&formatContext);
    }

    if (filterGraph) {
        avfilter_graph_free(&filterGraph);
    }

    initialized = false;
}

QString HighResProcessor::getProcessingStats() const {
    return QString("Processed Frames: %1/%2, FPS: %3, "
                  "Avg Processing Time: %4ms, Dropped Frames: %5")
        .arg(processedFrames)
        .arg(totalFrames)
        .arg(metrics.fps, 0, 'f', 1)
        .arg(metrics.averageProcessingTime, 0, 'f', 1)
        .arg(metrics.droppedFrames);
}

float HighResProcessor::getProcessingProgress() const {
    if (totalFrames <= 0) {
        return 0.0f;
    }
    return float(processedFrames) / float(totalFrames);
}

bool HighResProcessor::cancelProcessing() {
    processingCancelled = true;
    return true;
}

QString HighResProcessor::getErrorString(int error) const {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(error, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return QString::fromUtf8(errbuf);
}

void HighResProcessor::logError(const QString& error) {
    lastError = error;
    qDebug() << "HighResProcessor Error:" << error;
    emit errorOccurred(error);
}
