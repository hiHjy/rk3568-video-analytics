#ifndef INPUTFROMRTSP_H
#define INPUTFROMRTSP_H
#include <QThread>
#include <QObject>
#include "type.h"
#include <QTimer>
#include <QDateTime>
#include <queue>
#include <vector>

//#define TIMEOUT 10000000
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>

}




class Dialog;
class InputFromRTSP : public QObject
{
    Q_OBJECT
public:

    explicit InputFromRTSP(QObject *parent = nullptr, QString url = "");

    ~InputFromRTSP();
    static int interrupt_cb(void *opaque);
    void setInputNum(uint64_t inputNum);
    void setStop_flag(const std::atomic<bool> &newStop_flag);

signals:
    void yuvFrameReady(uchar *frame, uint width, uint height, InputStreamType type, uint64_t inputNum);
    void connectSuccess();
    void reInitRTSP();
    void reqInitDialog(QString msg);
    void reqConnDialog(QString msg);


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
    bool isFirst = true;
    bool isReady = true;
    QString url = "";
    std::queue<std::vector<uint8_t>> queue;
    void initRTSP(QString url);
    void releaseFFmpeg();
    QTimer *timer = nullptr;
    Dialog *dialog = nullptr;
    uint64_t inputNum = 0;






//    std::atomic<int64_t> last_data_us{0};   // 最后一次“成功读到数据”的时间（单调时钟）
//    int64_t timeout_us = TIMEOUT; // 5秒

};

#endif // INPUTFROMRTSP_H
