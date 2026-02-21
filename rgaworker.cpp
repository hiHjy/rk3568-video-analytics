#include "rgaworker.h"
#include <QDebug>
#include <QThread>
#include <QImage>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#define PERPIXBYTENUM 2
RGAWorker::RGAWorker(QObject *parent) : QObject(parent)
{



}

void RGAWorker::frameCvtColor(uchar* frame, uint32_t width, uint32_t height, InputStreamType type, uint64_t inputNum)
{

    int ret = -1;
    if (!RGBFrame || !encFrame || !yoloFrame) {
        RGBFrame = (char *)malloc(sizeof(char) * width * height * 3);
        yoloFrame = (char*)malloc(640*640*3);
        encFrame = (char*)malloc(sizeof(char) * width * height * 3 / 2);
        if (!RGBFrame || !encFrame) {
            perror("malloc");
            exit(-1);
        }
    }

    this->inputNum = inputNum;

    /***************** rga硬件加速****************************8 */
    switch (type) {
    case InputStreamType::LOCAL:
        src = wrapbuffer_virtualaddr((void*)frame, width, height, RK_FORMAT_YUYV_422, (int)width,(int) height);
        dst = wrapbuffer_virtualaddr((void*)RGBFrame, width, height, RK_FORMAT_RGB_888, (int)width, (int)height);



        ret = imcvtcolor(src, dst, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888);
        if (ret !=  IM_STATUS_SUCCESS) {
            qDebug() << "imcvtcolor" ;
            return;
        }
        break;

    case InputStreamType::RTSP:
        src = wrapbuffer_virtualaddr((void*)frame, width, height, RK_FORMAT_YCbCr_420_SP, (int)width,(int) height);
        dst = wrapbuffer_virtualaddr((void*)RGBFrame, width, height, RK_FORMAT_RGB_888, (int)width, (int)height);



        ret = imcvtcolor(src, dst, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_RGB_888);
        if (ret !=  IM_STATUS_SUCCESS) {
            qDebug() << "imcvtcolor" ;
            return;
        }
        break;
    }





    //yolo
    yoloRGB640X640  = wrapbuffer_virtualaddr((void*)yoloFrame, 640, 640, RK_FORMAT_RGB_888, 640,640);

    ret = immakeBorder(dst, yoloRGB640X640,
                       0, 160, 0, 0,     // top bottom left right
                       0, 0,             // border_type, value(black)
                       1, -1, NULL);     // sync=1
    if (ret != IM_STATUS_SUCCESS) {
        qDebug() << "immakeBorder failed:" << ret;
        return;
    }


    emit yoloRGB640X640Ready((uint8_t*)yoloFrame);



}



void RGAWorker::finalStep(detect_result_group_t* out)
{
    //qDebug() << "9999999999999999999999999999999999999999999999999999999999999999999999999999";
    yoloRGB640X640  = wrapbuffer_virtualaddr((void*)yoloFrame, 640, 480, RK_FORMAT_RGB_888, 640,480);
    dst_nv12 = wrapbuffer_virtualaddr((void*)encFrame, 640, 480, RK_FORMAT_YCbCr_420_SP, 640, 480);
    //计算fps
    static int frame_count = 0;
    static double fps = 0.0;
    static auto last_time = std::chrono::steady_clock::now();

    frame_count++;

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - last_time).count();

    if (elapsed >= 1.0) {
        fps = frame_count / elapsed;
        frame_count = 0;
        last_time = now;
    }

    //在显示的帧上画时间
    QDateTime date = QDateTime::currentDateTime();
    char osd[128];
    snprintf(osd, sizeof(osd), "%s         %.1ffps", date.toString("yyyy-MM-dd hh:mm:ss").toUtf8().data(), fps);
    cv::Mat tmp(480, 640, CV_8UC3, yoloFrame);
    cv::putText(tmp, osd, cv::Point(0, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);





    //在显示的帧上画yolo检测的框

    if (out->count > 0) {


        for (int i = 0; i < out->count; ++i) {
            detect_result_t* det = &out->results[i];
            if (det->prop < 60.0 / 100) continue; //置信度太低不画框

            int x1 = det->box.left;
            int y1 = det->box.top;
            int x2 = det->box.right;
            int y2 = det->box.bottom;

            //  画矩形框
            cv::rectangle(
                        tmp,
                        cv::Point(x1, y1),
                        cv::Point(x2, y2),
                        cv::Scalar(0, 255, 0),   // 绿色
                        2
                        );

            // 画文字（类别）
            char label[128];
            snprintf(label, sizeof(label), "%s",
                     det->name);

            cv::putText(
                        tmp,
                        label,
                        cv::Point(x1, y1 - 5),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.8,
                        cv::Scalar(255, 0, 255),
                        2
                        );
        }


    }


    //显示的帧准备就绪 送去本地qt显示
    emit displayFrameReady(yoloFrame, 640, 480, inputNum);





    //准备要送去给rkmpp编码的帧
    int ret = imcvtcolor(yoloRGB640X640, dst_nv12, RK_FORMAT_RGB_888, RK_FORMAT_YCbCr_420_SP);
    if (ret !=  IM_STATUS_SUCCESS) {
        qDebug() << "imcvtcolor" ;
        return;
    }

    //送去rkmpp编码推流
    emit encFrameReady(encFrame, 640, 480);

}

RGAWorker::~RGAWorker()
{
    if (!RGBFrame)
        free(RGBFrame);
    if (!yoloFrame)
        free(yoloFrame);
    if (!encFrame)
        free(encFrame);
}
