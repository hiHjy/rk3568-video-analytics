#include "inputfromrtsp.h"
#include <QDebug>
#include "dialog.h"
#include <atomic>


void print_error(const char *msg, int err)
{
    char buf[256];
    av_strerror(err, buf, sizeof(buf));
    qDebug() << msg << buf;
}

//逻辑可能有问题
int InputFromRTSP::interrupt_cb(void *opaque) {

    InputFromRTSP *self = static_cast<InputFromRTSP*>(opaque);
    //    //    static uint64_t n = 0;

    //    //    qDebug() << ++n;


    //    if (self->stop_flag.load(std::memory_order_relaxed))
    //        return 1;
    //    int64_t now = av_gettime_relative();
    //    int64_t last = self->last_data_us.load(std::memory_order_relaxed);
    //    if (last != 0 && (now - last) > self->timeout_us) {
    //        qDebug() << "RTSP超时，中断阻塞读";
    //        return 1;
    //    }
    //    qDebug() << "1";
    return self->stop_flag.load() ? 1 : 0;  // 1=中断阻塞
}

void InputFromRTSP::setInputNum(uint64_t inputNum)
{
    this->inputNum = inputNum;
}


InputFromRTSP::InputFromRTSP(QObject *parent, QString url) : QObject(parent), url(url)
{

    initRTSP(url);

    connect(this, &InputFromRTSP::reInitRTSP, this, [this]{

        qDebug() << "[INF] rtsp初始化失败,正在尝试重连";
        initRTSP(this->url);

        if (isReady) {
            timer->stop();




            decodeH264ToNV12();
        }

    });

}





InputFromRTSP::~InputFromRTSP()
{
    stop_flag.store(true);
    if (timer) {
        timer->stop();

    }
    releaseFFmpeg();
    //    if (yuvFrame) {

    //        free(yuvFrame);
    //        yuvFrame = nullptr;
    //    }

    qDebug() << "[rtsp worker] 被释放";
}

void InputFromRTSP::decodeH264ToNV12()
{
    int ret = -1;
    qDebug() << "isReady" << isReady;
    //int n = 0;
    if (!isReady) {
        qDebug() << "[ERR] InputFromRTSP Init Error";
        if (!timer) {
            timer = new QTimer(this);

        }

        emit reqInitDialog("初始化连接失败,地址可能不正确,正在尝试重新连接...");

        timer->start(3000);
        connect(timer, &QTimer::timeout, this, [this]{

            emit reInitRTSP();

        });

        return;
    }



    while (!QThread::currentThread()->isInterruptionRequested()) {



        //pthread_cond_broadcast(&cond);


        //ret =  错误 或者 文件末尾
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            qDebug() << "jjjjjj";
            emit reqConnDialog("连接中断,正在重新连接...");
            if (!timer) {
                timer = new QTimer(this);

            }
            timer->start(3000);
            connect(timer, &QTimer::timeout, this, [this]{

                emit reInitRTSP();

            });
            print_error("av_read_frame", ret);
            return;


        }


        //printf("packet stream_index:%d(video index:%d)\n", pkt->stream_index, video_index);
        //只处理视频流
        if (pkt->stream_index == video_index) {
            //pkt 是原始的h264数据，将这个原始数据拿去给dec_ctx去解码
            ret = avcodec_send_packet(dec_ctx, pkt);
            //qDebug() << "avcodec_send_packet ret=" << ret;
            if (ret != 0) {

                print_error("avcodec_send_packet", ret);
                return;
            }

            while (ret >= 0) {
                //“现在有没有一帧解码完成的数据？” 是 ret = 0
                ret = avcodec_receive_frame(dec_ctx, frame);

                //                    qDebug() << "dec_ctx pix_fmt=" << dec_ctx->pix_fmt
                //                             << av_get_pix_fmt_name((AVPixelFormat)dec_ctx->pix_fmt);

                //                    qDebug() << "frame format=" << frame->format
                //                             << av_get_pix_fmt_name((AVPixelFormat)frame->format);
                //qDebug() << "avcodec_receive_frame ret=" << ret;

                //解码阶段EAGAIN 需要更多 packet
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

                    break;
                }


                if (ret < 0) {
                    print_error("avcodec_receive_frame", ret);
                    return;
                }
                //printf("640*480*3/2=%d, frame_size=%d\n", 640*480*3/2, frame->width * frame->height * 3 / 2);

                //接收到数据,重置回调计时器
                //                last_data_us.store(av_gettime_relative());
                int w = frame->width;
                int h = frame->height;

                if (isFirst) {
                    isFirst = false;
                    nv12_contig.resize(w * h * 3 / 2);
                    emit connectSuccess();

                }
                // 4) 拷贝 Y：每行拷 w 字节（去掉 stride padding）
                for (int y = 0; y < h; ++y) {
                    memcpy(nv12_contig.data() + y * w,
                           frame->data[0] + y * frame->linesize[0],
                            w);
                }

                // 5) 拷贝 UV：h/2 行，每行 w 字节（NV12 的 UV 交错平面）
                uint8_t* dst_uv = nv12_contig.data() + w * h;
                for (int y = 0; y < h / 2; ++y) {
                    memcpy(dst_uv + y * w,
                           frame->data[1] + y * frame->linesize[1],
                            w);
                }





                //yuvFrame = (uchar*)frame->data[0];
                if (queue.size() == 3) {
                    queue.pop();
                    queue.push(nv12_contig);
                } else {
                    queue.push(nv12_contig);

                }
                if (!queue.empty() && !QThread::currentThread()->isInterruptionRequested()) {
                    emit yuvFrameReady(queue.front().data(), 640, 480, InputStreamType::RTSP, inputNum);

                }

                //程序到这里成功获取了一帧yuv420数据
                //printf("get frame:%dx%d pix_fmt=%d n=%d\n", frame->width, frame->height, frame->format, ++n);
                //return ret;
                av_frame_unref(frame);

            }

        }

        av_packet_unref(pkt); //表示购物车已经清空了，可以继续进货了

        if (QThread::currentThread()->isInterruptionRequested()) {
            avformat_network_deinit();
            break;
        }
    }



    qDebug() << "退出循环";





}

void InputFromRTSP::setStop_flag(const std::atomic<bool> &newStop_flag)
{
    stop_flag = newStop_flag.load();
}

void InputFromRTSP::initRTSP(QString url)
{



    if (!isReady) {

        releaseFFmpeg();
        isReady = true;
    }

    if (!isFirst) isFirst = true;
    avformat_network_init();
    //yuvFrame = (uchar *)malloc(640 * 480 * 3 /2);

    const AVCodec *decoder = avcodec_find_decoder_by_name("h264_rkmpp");
    if (!decoder) {
        isReady = false;
        qDebug() << " avcodec_find_decoder_by_name(\"h264_rkmpp\") error";
        return;
    }

    dec_ctx = avcodec_alloc_context3(decoder);
    if (!dec_ctx) {
        isReady = false;
        qDebug() << "avcodec_alloc_context3 error";
        return ;
    }


    AVDictionary *opts = NULL;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
//    av_dict_set(&opts, "stimeout", "5000000", 0); // 5秒，单位微秒
//    av_dict_set(&opts, "rw_timeout", "5000000", 0);  // 5s，读写超时
    av_dict_set(&opts, "timeout", "4000000", 0);
    fmt_ctx = avformat_alloc_context();

    //加下面两行代码，当无数据时会调用interrupt_cb函数，检查返回值，如果返回0那么会阻塞等待，如果返回1，则中断阻塞，ffmpeg底层用的poll
    //通过检查this下的某个标志为来确认用户是否需要中断阻塞，当用户要中断，会返回AVERROR_EXIT一个负数
    fmt_ctx ->interrupt_callback.callback = interrupt_cb;
    fmt_ctx ->interrupt_callback.opaque = this;

    qDebug() << url.toStdString().c_str();
    //    last_data_us.store(av_gettime_relative());
    int ret = avformat_open_input(&fmt_ctx, url.toStdString().c_str(), NULL, &opts);
    if (ret < 0) {
        isReady = false;
        print_error("avformat_open_input", ret);
        return ;
    }

    //=============查看ffmpeg支持的选项====================================
    //    AVDictionaryEntry *t = NULL;
    //    while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))) {
    //        qDebug() << "未使用的选项:" << t->key << "=" << t->value;
    //    }

    //    const AVOption* opt = nullptr;
    //    const AVClass* avclass = avformat_get_class();

    //    qDebug() << "=== FFmpeg 支持的选项 ===";
    //    while ((opt = av_opt_next(&avclass, opt))) {
    //        if (opt->name) {
    //            qDebug() << "选项:" << opt->name
    //                     << "类型:" << opt->type
    //                     << "帮助:" << (opt->help ? opt->help : "");
    //        }
    //    }


    qDebug() << "rtsp连接成功";
    // 解析流 ffmpeg -i input.mp4 输出的stream的相关行
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        isReady = false;
        print_error("avformat_find_stream_info", ret);
        return;
    }

    //输出有几个流 video and audio
    printf("nb_stream = %d\n", fmt_ctx->nb_streams);

    //遍历每个流
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        //获取每个流的指针
        AVStream *stream = fmt_ctx->streams[i];

        //获取这个流的的参数
        AVCodecParameters *acp = stream->codecpar;
        const char *type;
        switch (acp->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            /* code */
            type = "video";
            break;
        case AVMEDIA_TYPE_AUDIO:
            type = "audio";
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            type = "subtitle";

        default:
            type = "other";
            break;
        }
        printf("stream #%u: type:%s codec_id=%d\n", i, type, acp->codec_id);
        if (acp->codec_type == AVMEDIA_TYPE_VIDEO) {
            //printf("视频流: stream#%u %u * %u\n", i, acp->width, acp->height);
            video_index = i; //拿视频流的索引

        }



    }

    if (video_index < 0) {
        isReady = false;
        fprintf(stderr, "no video stream\n");
        return;
    }

    AVCodecParameters *video_acp = fmt_ctx->streams[video_index]->codecpar;  //拿到视频流的参数信息

    printf("extradata_size=%d\n", video_acp->extradata_size);
    if (video_acp->extradata_size > 0) {
        // 打印前16字节看看像不像 H264 配置
        for (int i = 0; i < video_acp->extradata_size && i < 16; i++)
            printf("%02X ", video_acp->extradata[i]);
        printf("\n");
    }

    //    char codec_name[32];
    //    memset(codec_name, 0, sizeof (codec_name));
    const char *codec_name = nullptr;
    codec_name = avcodec_get_name(video_acp->codec_id);          //拿视频流的编码格式
    printf("video stream index:%d  codec_name:%s\n", video_index, codec_name);


    //将视频流的参数信息交给解码器
    ret = avcodec_parameters_to_context(dec_ctx, video_acp);
    if (ret < 0) {
        isReady = false;
        print_error("avcodec_parameters_to_context error", ret);
        return;
    }


    //开启解码器
    ret = avcodec_open2(dec_ctx, decoder, NULL);
    if (ret < 0) {
        isReady = false;
        print_error("avcodec_open2 error", ret);
        return;
    }


    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    if (!pkt || !frame) {
        isReady = false;
        fprintf(stderr, "av_packet_alloc or av_frame_alloc error\n");
        return;
    }


    //走到这里已经启动了视频解码器
    printf("decoder opened: %s\n", decoder->name);
    printf("decoded video: size=%d * %d pix_fmt=%d\n", dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt);

    qDebug() << "[rtsp pull] --h264_rkmpp-- 解码器初始化完成";


}

void InputFromRTSP::releaseFFmpeg()
{

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx); // 会释放内部结构，并把 fmt_ctx 置空
    }

    if (dec_ctx) {
        avcodec_free_context(&dec_ctx);
    }
    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);

}


