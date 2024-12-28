#include "mediaprocessor.h"
#include <QFileInfo>

QString MediaProcessor::extractMetadata(const QString& filePath)
{
    AVFormatContext* formatContext = nullptr;
    QString metadata;

    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) < 0) {
        return "Error: Could not open file";
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return "Error: Could not find stream information";
    }

    metadata = "File: " + QFileInfo(filePath).fileName() + "\n";
    metadata += "Duration: " + formatDuration(formatContext->duration) + "\n";
    metadata += "Number of streams: " + QString::number(formatContext->nb_streams) + "\n\n";

    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream* stream = formatContext->streams[i];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        
        metadata += "Stream #" + QString::number(i) + "\n";
        metadata += "Type: " + QString(av_get_media_type_string(stream->codecpar->codec_type)) + "\n";
        if (codec) {
            metadata += "Codec: " + QString(codec->name) + "\n";
        }

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            metadata += QString("Resolution: %1x%2\n")
                .arg(stream->codecpar->width)
                .arg(stream->codecpar->height);
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata += QString("Sample Rate: %1 Hz\n")
                .arg(stream->codecpar->sample_rate);
            metadata += QString("Channels: %1\n")
                .arg(stream->codecpar->ch_layout.nb_channels);
        }
        metadata += "\n";
    }

    avformat_close_input(&formatContext);
    return metadata;
}

bool MediaProcessor::exportFile(const QString& inputPath, const QString& outputPath)
{
    QString command = QString("ffmpeg -i \"%1\" -c copy \"%2\"")
        .arg(inputPath)
        .arg(outputPath);
    
    return system(command.toStdString().c_str()) == 0;
}

QString MediaProcessor::formatDuration(int64_t duration)
{
    if (duration < 0) {
        return "N/A";
    }

    int64_t seconds = duration / AV_TIME_BASE;
    int64_t hours = seconds / 3600;
    seconds %= 3600;
    int64_t minutes = seconds / 60;
    seconds %= 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
