#include "rgaworker.h"
#include <QDebug>
#include <QThread>
#define PERPIXBYTENUM 2
RGAWorker::RGAWorker(QObject *parent) : QObject(parent)
{


}

void RGAWorker::frameCvtColor(uchar* frame, uint32_t width, uint32_t height)
{


    if (!displayFrame && !encFrame) {
       displayFrame = (char *)malloc(sizeof(char) * width * height * 3);
       encFrame = (char*)malloc(sizeof(char) * width * height * 3 / 2);
       if (!displayFrame || !encFrame) {
           perror("malloc");
           exit(-1);
       }
    }




    rga_buffer_t src = wrapbuffer_virtualaddr((void*)frame, width, height, RK_FORMAT_YUYV_422, (int)width,(int) height);
    rga_buffer_t dst = wrapbuffer_virtualaddr((void*)displayFrame, width, height, RK_FORMAT_RGB_888, (int)width, (int)height);
    rga_buffer_t dst_nv12 = wrapbuffer_virtualaddr((void*)encFrame, width, height, RK_FORMAT_YCrCb_420_SP, (int)width, (int)height);
    int ret = imcvtcolor(src, dst, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888);
    if (ret !=  IM_STATUS_SUCCESS) {
        qDebug() << "imcvtcolor" ;
        return;
    }
    ret = imcvtcolor(src, dst_nv12, RK_FORMAT_YUYV_422, RK_FORMAT_YCrCb_420_SP);
    if (ret !=  IM_STATUS_SUCCESS) {
        qDebug() << "imcvtcolor" ;
        return;
    }

    emit displayFrameReady(displayFrame, width, height);
    emit encFrameReady(encFrame, width, height);
}

RGAWorker::~RGAWorker()
{
    free(displayFrame);
}
