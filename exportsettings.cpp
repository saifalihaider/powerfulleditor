#include "exportsettings.h"

ExportSettings::ExportSettings()
    : resolution(1920, 1080)
    , frameRate(30)
    , videoCodec(VideoCodec::H264)
    , videoBitrate(5000)  // 5 Mbps
    , crf(23)
    , preset("medium")
    , audioEnabled(true)
    , audioCodec(AudioCodec::AAC)
    , audioBitrate(192)   // 192 kbps
    , sampleRate(48000)   // 48 kHz
    , containerFormat(ContainerFormat::MP4)
{
}

QString ExportSettings::getVideoCodecString() const {
    switch (videoCodec) {
        case VideoCodec::H264:
            return "libx264";
        case VideoCodec::H265:
            return "libx265";
        case VideoCodec::VP9:
            return "libvpx-vp9";
        case VideoCodec::AV1:
            return "libaom-av1";
        default:
            return "libx264";
    }
}

QString ExportSettings::getAudioCodecString() const {
    switch (audioCodec) {
        case AudioCodec::AAC:
            return "aac";
        case AudioCodec::MP3:
            return "libmp3lame";
        case AudioCodec::OPUS:
            return "libopus";
        case AudioCodec::VORBIS:
            return "libvorbis";
        default:
            return "aac";
    }
}

QString ExportSettings::getContainerFormatString() const {
    switch (containerFormat) {
        case ContainerFormat::MP4:
            return "mp4";
        case ContainerFormat::MOV:
            return "mov";
        case ContainerFormat::MKV:
            return "matroska";
        case ContainerFormat::AVI:
            return "avi";
        case ContainerFormat::WEBM:
            return "webm";
        default:
            return "mp4";
    }
}

QString ExportSettings::getFileExtension() const {
    switch (containerFormat) {
        case ContainerFormat::MP4:
            return ".mp4";
        case ContainerFormat::MOV:
            return ".mov";
        case ContainerFormat::MKV:
            return ".mkv";
        case ContainerFormat::AVI:
            return ".avi";
        case ContainerFormat::WEBM:
            return ".webm";
        default:
            return ".mp4";
    }
}

QStringList ExportSettings::getFFmpegParameters() const {
    QStringList params;
    
    // Video codec and settings
    params << "-c:v" << getVideoCodecString();
    
    // Video bitrate or CRF
    if (videoBitrate > 0) {
        params << "-b:v" << QString("%1k").arg(videoBitrate);
    } else {
        params << "-crf" << QString::number(crf);
    }
    
    // Preset
    params << "-preset" << preset;
    
    // Frame rate
    params << "-r" << QString::number(frameRate);
    
    // Resolution
    params << "-s" << QString("%1x%2").arg(resolution.width()).arg(resolution.height());
    
    // Audio settings
    if (audioEnabled) {
        params << "-c:a" << getAudioCodecString();
        params << "-b:a" << QString("%1k").arg(audioBitrate);
        params << "-ar" << QString::number(sampleRate);
    } else {
        params << "-an";  // Disable audio
    }
    
    // Container format
    params << "-f" << getContainerFormatString();
    
    return params;
}
