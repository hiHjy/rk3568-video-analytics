//querystring： 查询获取当前芯片平台RGA硬件版本与功能支持信息，以字符串的形式返回。
//importbuffer_T： 将外部buffer导入RGA驱动内部，实现硬件快速访问非连续物理地址（dma_fd、虚拟地址）。
//releasebuffer_handle： 将外部buffer从RGA驱动内部解除引用与映射。
//wrapbuffer_handle速封装图像缓冲区结构（rga_buffer_t）。
//imcopy： 调用RGA实现快速图像拷贝操作。
//imresize： 调用RGA实现快速图像缩放操作。
//impyramind： 调用RGA实现快速图像金字塔操作。
//imcrop： 调用RGA实现快速图像裁剪操作。
//imrotate： 调用RGA实现快速图像旋转操作。
//imflip： 调用RGA实现快速图像翻转操作。
//imfill： 调用RGA实现快速图像填充操作。
//imtranslate： 调用RGA实现快速图像平移操作。
//imblend： 调用RGA实现双通道快速图像合成操作。
//imcomposite： 调用RGA实现三通道快速图像合成操作。
//imcolorkey： 调用RGA实现快速图像颜色键操作。
//imcvtcolor： 调用RGA实现快速图像格式转换。
//imquantize： 调用RGA实现快速图像运算点前处理（量化）操作。
//imrop： 调用RGA实现快速图像光栅操作。
//improcess： 调用RGA实现快速图像复合处理操作。
//imcheck： 校验参数是否合法，以及当前硬件是否支持该操作。
//imsync： 用于异步模式时，同步任务完成状态。
//imconfig： 向当前线程上下文添加默认配置。

#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <rgaworker.h>
#include <mppworker.h>
#include <yoloworker.h>
#include <inputfromrtsp.h>
#include "inputmanager.h"
#include <QAbstractItemView>
#include <QListView>
#include <streaminfo.h>
#include <QDialog>
#include "dialog.h"
class CamWorker;
Widget* Widget::self = nullptr;
Widget *Widget::getInstance()
{
    return self;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    qDebug() << "主线程:" << QThread::currentThread();





    ui->setupUi(this);

    QListView * listView = new QListView(ui->comboBox);;
    ui->comboBox->setView(listView);

    ui->btn_local->clicked(true);

    self = this;



    rgaT = new QThread(this);
    RGA = new RGAWorker();
    RGA->moveToThread(rgaT);



    MPP = new MPPWorker(640, 480);
    mppT = new QThread(this);
    MPP->moveToThread(mppT);


    YOLO = new YOLOWorker();
    yoloT = new QThread(this);
    YOLO->moveToThread(yoloT);




    connect(RGA, &RGAWorker::displayFrameReady, this, &Widget::localDisplay);
    connect(rgaT, &QThread::finished, RGA, &QObject::deleteLater);

    connect(RGA, &RGAWorker::encFrameReady, MPP, &MPPWorker::encode2H264);
    connect(mppT, &QThread::finished, MPP, &QObject::deleteLater);

    connect(RGA, &RGAWorker::yoloRGB640X640Ready, YOLO, &YOLOWorker::inferRgb640);

    connect(YOLO, &YOLOWorker::drawRectReady, RGA, &RGAWorker::finalStep);
    connect(yoloT, &QThread::finished, YOLO, &QObject::deleteLater);

    rgaT->start();
    mppT->start();
    yoloT->start();




    InputManager *inputManager = new InputManager(this, InputStreamType::LOCAL, "rtsp://192.168.1.19:8554/live", RGA);
    //    streamInfo = new StreamInfo(ui->widget_input_select);


    //    streamInfo->setAttribute(Qt::WA_StyledBackground, true);
    //    streamInfo->setAutoFillBackground(true);


    //    streamInfo->raise();
    //    streamInfo->show();

    //connect(this, &Widget::selectInputStream, inputManager, &InputManager::setInputMode);
    connect(this, &Widget::selectInputStream, inputManager, &InputManager::setInputStream);
    connect(inputManager, &InputManager::requestClear, this, [this](){

        this->enableDisplay = false;
        ui->lb_img->clear();
        ui->lb_img->setStyleSheet("background:rgb(37,40,48);");

    });

}


Widget::~Widget()
{
    //    if (camT && camT->isRunning()) {

    //        camT->requestInterruption();
    //        camT->quit();
    //        if (camT->wait(3000)) {
    //            qDebug() << "采集线程正常结束";
    //        } else {
    //            qDebug() << "采集线程还在运行，没有正常结束";
    //        }
    //    }

    if (rgaT && rgaT->isRunning()) {


        rgaT->quit();
        if (rgaT->wait(3000)) {
            qDebug() << "rga线程正常结束";
        } else {
            qDebug() << "rga线程还在运行，没有正常结束";
        }
    }

    if (mppT && mppT->isRunning()) {


        mppT->quit();
        if (mppT->wait(3000)) {
            qDebug() << "mpp线程正常结束";
        } else {
            qDebug() << "mpp线程还在运行，没有正常结束";
        }
    }

    if (yoloT && yoloT->isRunning()) {


        yoloT->quit();
        if (yoloT->wait(3000)) {
            qDebug() << "yolo线程正常结束";
        } else {
            qDebug() << "yolo线程还在运行，没有正常结束";
        }
    }
    delete ui;
}

void Widget::localDisplay(char *displayFramePtr, int width, int height)
{

    if (!enableDisplay) {
        return;
    }
    QImage img((uchar*)displayFramePtr, width, height, QImage::Format_RGB888);
    ui->lb_img->setPixmap(QPixmap::fromImage(img));



}


Worker::Worker(QObject *parent) :QThread(parent)
{

}

Worker::~Worker()
{

}
#if 0 //Not using RGA hardware acceleration
void Worker::run()
{
    qDebug() << "worker start";
    avdevice_register_all();

    AVDictionary *opt = nullptr;
    AVFormatContext *in_ctx = avformat_alloc_context();
    /**以下平台不同可能需要修改 */
    av_dict_set(&opt, "framerate", "30", 0);
    av_dict_set(&opt, "video_size", "640x480", 0);
    av_dict_set(&opt, "pixel_format", "yuyv422", 0);
    int ret = avformat_open_input(&in_ctx, "/dev/video10", av_find_input_format("v4l2"), &opt);
    av_dict_free(&opt);
    if (ret < 0) { print_error("avformat_open_input", ret); return; }

    ret = avformat_find_stream_info(in_ctx, nullptr);
    if (ret < 0) { print_error("avformat_find_stream_info", ret); return; }

    int video_index = -1;
    for (unsigned i = 0; i < in_ctx->nb_streams; ++i) {
        if (in_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = (int)i;
            break;
        }
    }
    AVCodecParameters *in_par = in_ctx->streams[video_index]->codecpar;
    printf("size:%d*%d format:%s\n", in_par->width, in_par->height, av_get_pix_fmt_name((AVPixelFormat)in_par->format));

    SwsContext *yuyv2rgb = sws_getContext(in_par->width, in_par->height, (AVPixelFormat)in_par->format,in_par->width, in_par->height, AV_PIX_FMT_RGB24,
                                          SWS_BILINEAR, nullptr, nullptr, nullptr);



    if (video_index < 0) { std::cerr << "no video stream\n"; return; }
    AVFrame *frame = av_frame_alloc ();
    AVPacket *pkt = av_packet_alloc();
    QImage img(640, 480, QImage::Format_RGB888);
    uint8_t *data[4] = {img.bits(), nullptr, nullptr, nullptr};
    int linesize[4] = {img.bytesPerLine(), 0, 0, 0};
    if (!frame || !pkt) {
        qDebug() << "av_frame_alloc or av_packet_alloc";
        //imresize();
    }
    while (true) {
        int ret = av_read_frame(in_ctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
            usleep(5000);
            continue;
        }
        frame->format = in_par->format;
        frame->width = in_par->width;
        frame->height = in_par->height;
        frame->data[0] = pkt->data;
        frame->linesize[0] = in_par->width *2;
        ret = sws_scale(yuyv2rgb, frame->data, frame->linesize, 0, in_par->height, data, linesize);
        if (ret <= 0) {
            std::cerr << "sws_scale error" << std::endl;
            return ;
        }
        Widget::getInstance()->ui->label->setPixmap(QPixmap::fromImage(img));
        av_packet_unref(pkt);


    }


}
#endif

#if 1 // using RGA hardware acceleration
void Worker::run()
{
    qDebug() << "worker start";
    //    avdevice_register_all();



    //    AVDictionary *opt = nullptr;
    //    AVFormatContext *in_ctx = avformat_alloc_context();

    //    /**以下平台不同可能需要修改 */
    //    av_dict_set(&opt, "framerate", "30", 0);
    //    av_dict_set(&opt, "video_size", "640x480", 0);
    //    av_dict_set(&opt, "pixel_format", "yuyv422", 0);
    //    int ret = avformat_open_input(&in_ctx, "/dev/video10", av_find_input_format("v4l2"), &opt);
    //    av_dict_free(&opt);
    //    if (ret < 0) { print_error("avformat_open_input", ret); return; }

    //    ret = avformat_find_stream_info(in_ctx, nullptr);
    //    if (ret < 0) { print_error("avformat_find_stream_info", ret); return; }

    //    int video_index = -1;
    //    for (unsigned i = 0; i < in_ctx->nb_streams; ++i) {
    //        if (in_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    //            video_index = (int)i;
    //            break;
    //        }
    //    }
    //    AVCodecParameters *in_par = in_ctx->streams[video_index]->codecpar;
    //    printf("size:%d*%d format:%s\n", in_par->width, in_par->height, av_get_pix_fmt_name((AVPixelFormat)in_par->format));


    //    /*
    //    IM_STATUS imcvtcolor(rga_buffer_t src,
    //    rga_buffer_t dst,
    //    int sfmt,
    //    int dfmt,
    //    int mode = IM_COLOR_SPACE_DEFAULT,
    //    int sync = 1)

    //    */





    //    if (video_index < 0) { std::cerr << "no video stream\n"; return; }
    //    AVFrame *frame = av_frame_alloc ();
    //    AVPacket *pkt = av_packet_alloc();
    //    QImage img(640, 480, QImage::Format_RGB888);

    //    if (!frame || !pkt) {
    //        qDebug() << "av_frame_alloc or av_packet_alloc";
    //        //imresize();
    //    }




    //    while (true) {

    //        int ret = av_read_frame(in_ctx, pkt);
    //        if (ret == AVERROR(EAGAIN)) {
    //            usleep(5000);
    //            continue;
    //        }
    //        /*
    //            第五个参数是,每一行多少像素,包括内存对齐

    //        */
    //        rga_buffer_t src = wrapbuffer_virtualaddr((void*)pkt->data, in_par->width, in_par->height, RK_FORMAT_YUYV_422, in_par->width, in_par->height);
    //        rga_buffer_t dst = wrapbuffer_virtualaddr((void*)img.bits(), img.width(), img.height(), RK_FORMAT_RGB_888, img.bytesPerLine()/3, img.height());


    //        ret = imcvtcolor(src, dst, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888);
    //        if (ret !=  IM_STATUS_SUCCESS) {
    //            qDebug() << "imcvtcolor" ;
    //            return;
    //        }

    //        Widget::getInstance()->ui->label->setPixmap(QPixmap::fromImage(img));
    //        av_packet_unref(pkt);


    //    }


}
#endif

void Widget::on_btn_local_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->btn_remote->setChecked(false);
    ui->btn_local->setChecked(true);

}


void Widget::on_btn_remote_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->btn_local->setChecked(false);
    ui->btn_remote->setChecked(true);
}


void Widget::on_btn_start_cam_clicked()
{
    enableDisplay = true;
    emit selectInputStream(InputStreamType::LOCAL, "本地", "", ui->widget_input_select);

}



void Widget::on_btn_remote_conn_clicked()
{

    enableDisplay = true;
    QString protocol = ui->comboBox->currentText();
    QString url = "";

    if (protocol == "RTSP") {

        url = "rtsp://" + ui->le_addr->text().trimmed();
        qDebug() << "url:" << url;


        //emit selectInputStream(InputStreamType::RTSP, url);
        //QThread::msleep(20);

        emit selectInputStream(InputStreamType::RTSP, "远程", url, ui->widget_input_select);


    } else if (protocol == "RTMP") {
        //todo

    }






}



