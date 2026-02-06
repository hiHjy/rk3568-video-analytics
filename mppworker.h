#ifndef MPPWORKER_H
#define MPPWORKER_H

#include <QObject>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>

}
class MPPWorker : public QObject
{
    Q_OBJECT
public:
    MPPWorker(int w = -1, int h = -1);

signals:
public slots:

    void encode2H264(char *nv12Frame, int width, int height);

private:
    int width = 0;
    int height = 0;
    uint64_t pts = 0;
    void rtspInit();
    AVCodecContext *enc_ctx = nullptr;
    AVFrame *enc_frame = nullptr;
    AVPacket *enc_pkt;
    AVFormatContext *rtsp_ctx;
    AVStream *rtsp_stream;
    void print_error(const char *msg, int err);

    ~MPPWorker();
};

#endif // MPPWORKER_H
