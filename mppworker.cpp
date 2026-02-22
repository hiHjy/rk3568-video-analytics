#include "mppworker.h"
#include <QDebug>
#define FPS 30
#define RTSP_URL
MPPWorker::MPPWorker(int w, int h)
    : width(w), height(h)

{
    initCoder();
    rtspInit();





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

        //qDebug() << "编码获得第 "<< enc_pkt->pts << "个packet size:" << enc_pkt->size << " dts:" << enc_pkt->dts;
        enc_pkt->stream_index = rtsp_stream->index;

        //时间戳换算
        av_packet_rescale_ts(
                    enc_pkt,
                    enc_ctx->time_base,
                    rtsp_stream->time_base
                    );

        ret = av_interleaved_write_frame(rtsp_ctx, enc_pkt); //推流
        if (ret < 0) {
            print_error("av_interleaved_write_frame", ret);
            releaseFFmpeg();
            initCoder();
            rtspInit();

            return ;
        }

        //打包了一个packet








    }

    av_packet_unref(enc_pkt);





}

void MPPWorker::rtspInit()
{
    //** 创建rtsp输出上下文 */
    rtsp_ctx = nullptr;
    avformat_alloc_output_context2(&rtsp_ctx ,nullptr, "rtsp", "rtsp://127.0.0.1:8554/live");
    if (!rtsp_ctx) {
        qDebug() << "[rtspInit]avformat_alloc_output_context2 failed";
        return;
    }

    //给这个rtsp格式的文件创建一个新流
    rtsp_stream = avformat_new_stream(rtsp_ctx, nullptr);
    if (!rtsp_stream) {
        qDebug() << "[rtspInit]avformat_new_stream failed";
        return ;
    }

    if (!enc_ctx) {
        qDebug() << "[rtspInit]enc_ctx is null";
        return;
    }

    int ret = avcodec_parameters_from_context(rtsp_stream->codecpar, enc_ctx);
    if (ret < 0) {
        print_error("avcodec_parameters_from_context", ret);
        return ;
    }
    rtsp_stream->time_base = enc_ctx->time_base;

    //   rtsp的参数
    AVDictionary *rtsp_opts = nullptr;

    av_dict_set(&rtsp_opts, "rtsp_transport", "tcp", 0);

    //    // 关键：先 open，再写 header
    //    ret = avio_open2(&rtsp_ctx->pb, "rtsp://127.0.0.1:8554/live", AVIO_FLAG_WRITE, nullptr, &rtsp_opts);
    //    if (ret < 0) { print_error("avio_open2", ret); return; }

    //将容器rtsp的头部信息（Header） 写入文件。
    ret = avformat_write_header(rtsp_ctx, &rtsp_opts);
    if (ret < 0) {
        print_error("avformat_write_header", ret);
        return;
    }
    qDebug() << "RTSP Init Successfully";
}


void MPPWorker::print_error(const char *msg, int err)
{
    char buf[256];
    av_strerror(err, buf, sizeof(buf));
    qDebug() << msg << buf;
}

void MPPWorker::releaseFFmpeg()
{
    if (enc_pkt)
        av_packet_free(&enc_pkt);
    if (enc_frame)
        av_frame_free(&enc_frame);
    if (enc_ctx)
        avcodec_free_context(&enc_ctx);
}

void MPPWorker::initCoder()
{
    avformat_network_init();
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
    enc_ctx->pix_fmt = AV_PIX_FMT_NV12;
    enc_ctx->time_base = {1, FPS};
    enc_ctx->framerate = {FPS, 1};
    enc_ctx->gop_size = FPS;
    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 全局头（RTSP必需）
    enc_ctx->bit_rate = 3 * 1000 * 1000; //不知道是啥
    enc_ctx->max_b_frames = 0; //关闭B帧
    av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(enc_ctx->priv_data, "preset", "veryfast", 0);
    //av_opt_set(enc_ctx->priv_data, "x264-params", "repeat-headers=1:aud=1:scenecut=0", 0);
    int ret = avcodec_open2(enc_ctx, encoder, NULL); //启动
    if (ret < 0) {
        print_error("avcodec_open2", ret);
        return;
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

MPPWorker::~MPPWorker()
{
    releaseFFmpeg();
    qDebug() << " 编码器工作线程正常退出";

}
