#include "mppworker.h"
#include <QDebug>
MPPWorker::MPPWorker(QObject *parent) : QObject(parent)
{
    const AVCodec *encoder = avcodec_find_encoder_by_name("h264_rkmpp");
    if (!encoder) {
        qDebug() << " avcodec_find_encoder_by_name(\"h264_rkmpp\") error";
        return;
    }
}

void MPPWorker::encode2H264(char *nv12Frame, int width, int height)
{

}
void MPPWorker::print_error(const char *msg, int err)
{
    char buf[256];
    av_strerror(err, buf, sizeof(buf));
    qDebug() << buf;
}
