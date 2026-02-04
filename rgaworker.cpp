#include "rgaworker.h"
#include <QDebug>
#include <QThread>
#include <QImage>
#define PERPIXBYTENUM 2
RGAWorker::RGAWorker(QObject *parent) : QObject(parent)
{



}

void RGAWorker::frameCvtColor(uchar* frame, uint32_t width, uint32_t height)
{

    if (!RGBFrame || !encFrame || !yoloFrame) {
        RGBFrame = (char *)malloc(sizeof(char) * width * height * 3);
        yoloFrame = (char*)malloc(640*640*3);
        encFrame = (char*)malloc(sizeof(char) * width * height * 3 / 2);
        if (!RGBFrame || !encFrame) {
            perror("malloc");
            exit(-1);
        }
    }


    /***************** rga硬件加速****************************8 */

    rga_buffer_t src = wrapbuffer_virtualaddr((void*)frame, width, height, RK_FORMAT_YUYV_422, (int)width,(int) height);
    rga_buffer_t dst = wrapbuffer_virtualaddr((void*)RGBFrame, width, height, RK_FORMAT_RGB_888, (int)width, (int)height);
    rga_buffer_t dst_nv12 = wrapbuffer_virtualaddr((void*)encFrame, width, height, RK_FORMAT_YCrCb_420_SP, (int)width, (int)height);


    int ret = imcvtcolor(src, dst, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888);
    if (ret !=  IM_STATUS_SUCCESS) {
        qDebug() << "imcvtcolor" ;
        return;
    }

    //yolo
    rga_buffer_t yoloRGB640X640 = wrapbuffer_virtualaddr((void*)yoloFrame, 640, 640, RK_FORMAT_RGB_888, 640,640);

    ret = immakeBorder(dst, yoloRGB640X640,
                           0, 160, 0, 0,     // top bottom left right
                           0, 0,             // border_type, value(black)
                           1, -1, NULL);     // sync=1
    if (ret != IM_STATUS_SUCCESS) {
        qDebug() << "immakeBorder failed:" << ret;
        return;
    }


    emit yoloRGB640X640Ready((uint8_t*)yoloFrame);



    ret = imcvtcolor(src, dst_nv12, RK_FORMAT_YUYV_422, RK_FORMAT_YCrCb_420_SP);
    if (ret !=  IM_STATUS_SUCCESS) {
        qDebug() << "imcvtcolor" ;
        return;
    }


    emit encFrameReady(encFrame, width, height);
}

void RGAWorker::finalStep()
{

    emit displayFrameReady(yoloFrame, 640, 480);
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
