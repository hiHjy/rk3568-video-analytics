#ifndef INPUTFROMRTSP_H
#define INPUTFROMRTSP_H

#include <QObject>
#include "type.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>


}
class InputFromRTSP : public QObject
{
    Q_OBJECT
public:
    explicit InputFromRTSP(QObject *parent = nullptr, QString url = "");
   ~InputFromRTSP();
    static int interrupt_cb(void *opaque);
signals:
    void yuvFrameReady(uchar *frame, uint width, uint height, InputStreamType type);
public slots:
    void decodeH264ToNV12();

private:

     AVCodecContext *dec_ctx = nullptr;
     AVFormatContext *fmt_ctx = nullptr;
     AVStream *rtsp_stream = nullptr;
     AVFrame  *frame = nullptr;
     AVPacket *pkt = nullptr;
     uchar *yuvFrame = nullptr;
     int video_index = -1;
     std::atomic<bool> stop_flag{false};
     std::vector<uint8_t> nv12_contig;


};

#endif // INPUTFROMRTSP_H
