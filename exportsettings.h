#pragma once

#include <QString>
#include <QSize>

enum class VideoCodec {
    H264,
    H265,
    VP9,
    AV1
};

enum class AudioCodec {
    AAC,
    MP3,
    OPUS,
    VORBIS
};

enum class ContainerFormat {
    MP4,
    MOV,
    MKV,
    AVI,
    WEBM
};

class ExportSettings {
public:
    ExportSettings();
    
    // Video settings
    QSize getResolution() const { return resolution; }
    int getFrameRate() const { return frameRate; }
    VideoCodec getVideoCodec() const { return videoCodec; }
    int getVideoBitrate() const { return videoBitrate; }
    int getCRF() const { return crf; }
    QString getPreset() const { return preset; }
    
    void setResolution(const QSize& res) { resolution = res; }
    void setFrameRate(int fps) { frameRate = fps; }
    void setVideoCodec(VideoCodec codec) { videoCodec = codec; }
    void setVideoBitrate(int bitrate) { videoBitrate = bitrate; }
    void setCRF(int value) { crf = value; }
    void setPreset(const QString& value) { preset = value; }
    
    // Audio settings
    bool isAudioEnabled() const { return audioEnabled; }
    AudioCodec getAudioCodec() const { return audioCodec; }
    int getAudioBitrate() const { return audioBitrate; }
    int getSampleRate() const { return sampleRate; }
    
    void setAudioEnabled(bool enabled) { audioEnabled = enabled; }
    void setAudioCodec(AudioCodec codec) { audioCodec = codec; }
    void setAudioBitrate(int bitrate) { audioBitrate = bitrate; }
    void setSampleRate(int rate) { sampleRate = rate; }
    
    // Container format
    ContainerFormat getContainerFormat() const { return containerFormat; }
    void setContainerFormat(ContainerFormat format) { containerFormat = format; }
    
    // Helper functions
    QString getVideoCodecString() const;
    QString getAudioCodecString() const;
    QString getContainerFormatString() const;
    QString getFileExtension() const;
    
    // Generate FFmpeg parameters
    QStringList getFFmpegParameters() const;

private:
    // Video settings
    QSize resolution;
    int frameRate;
    VideoCodec videoCodec;
    int videoBitrate;  // in kbps
    int crf;           // Constant Rate Factor (quality)
    QString preset;    // FFmpeg preset (e.g., ultrafast, fast, medium, slow)
    
    // Audio settings
    bool audioEnabled;
    AudioCodec audioCodec;
    int audioBitrate;  // in kbps
    int sampleRate;    // in Hz
    
    // Container format
    ContainerFormat containerFormat;
};
