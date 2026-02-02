#include "mppworker.h"
#include <QDebug>
#define FPS 30
MPPWorker::MPPWorker(int w, int h)
    : width(w), height(h)
{
    const AVCodec *encoder = avcodec_find_encoder_by_name("h264_rkmpp");
    if (!encoder) {
        qDebug() << " avcodec_find_encoder_by_name(\"h264_rkmpp\") error";
        return;
    }


    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        qDebug() << "avcodec_alloc_context3 error";
        return ;
    }
    enc_ctx->width = width;
    enc_ctx->height = height;
    enc_ctx->pix_fmt = AV_PIX_FMT_NV12; //编码器得是这个格式，可用于mp4，rtsp
    enc_ctx->time_base = {1, FPS};
    enc_ctx->framerate = {FPS, 1};

    enc_ctx->bit_rate = 800000; //不知道是啥
    enc_ctx->max_b_frames = 0; //关闭B帧
    av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(enc_ctx->priv_data, "preset", "veryfast", 0);
    int ret = avcodec_open2(enc_ctx, encoder, NULL); //启动
    if (ret < 0) {
        print_error("avcodec_open2", ret);

    }
    //编码要用的buf
    enc_frame = av_frame_alloc();
    if (!enc_frame) {
        qDebug() << "av_frame_alloc() error";
        return;
    }
    enc_frame->pts = 0;

    enc_pkt = av_packet_alloc();
    if (!enc_pkt) {
        qDebug() << "av_packet_alloc() error";
        return;
    }
    qDebug() << "编码器启动成功！";



}

void MPPWorker::encode2H264(char *nv12Frame, int width, int height)
{
    int ret = -1;
    enc_frame->width = width;
    enc_frame->height = height;
    enc_frame->format = enc_ctx->pix_fmt;
    enc_frame->data[0] = (uint8_t*)nv12Frame;

    enc_frame->data[1] = (uint8_t*)nv12Frame + width * height;

    // linesize 是每行字节数（无 padding 时就是 width）
    enc_frame->linesize[0] = width;
    enc_frame->linesize[1] = width;


    enc_frame->pts++;
    ret = avcodec_send_frame(enc_ctx, enc_frame);//编码将AVFrame 一帧的数据交给编码器编码
    if (ret < 0) {
        print_error("avcodec_send_frame", ret);
        return;
    }

    while (true) {

        ret = avcodec_receive_packet(enc_ctx, enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

            break;
        }

        if (ret < 0) {
            print_error("avcodec_receive_packet error", ret);
            return;
        }

        //打包了一个packet

        qDebug() << "编码获得第 "<< enc_pkt->pts << "个packet size:" << enc_pkt->size << " dts:" << enc_pkt->dts;






    }

    av_packet_unref(enc_pkt);





}


void MPPWorker::print_error(const char *msg, int err)
{
    char buf[256];
    av_strerror(err, buf, sizeof(buf));
    qDebug() << msg << buf;
}

MPPWorker::~MPPWorker()
{
    av_packet_free(&enc_pkt);
    av_frame_free(&enc_frame);
    avcodec_free_context(&enc_ctx);
    qDebug() << " 编码器工作线程正常退出";
}
