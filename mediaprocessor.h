#pragma once

#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class MediaProcessor {
public:
    MediaProcessor() = default;
    ~MediaProcessor() = default;

    QString extractMetadata(const QString& filePath);
    bool exportFile(const QString& inputPath, const QString& outputPath);

private:
    QString formatDuration(int64_t duration);
};
