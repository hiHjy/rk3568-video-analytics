#ifndef CAMWORKER_H
#define CAMWORKER_H

#include <QObject>
#include <QDebug>
#include <QMutex>

extern "C" {
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <poll.h>
}

struct cam_buf {
    unsigned long length;
    unsigned char* start;
};

typedef struct camera_format {

    char description[32]; //字符串描述信息
    int pixelformat; //像素格式


} cam_fmt;
class CamWorker : public QObject
{
    Q_OBJECT
public:
    explicit CamWorker(QObject *parent = nullptr);
    struct cam_buf *buf_infos = nullptr;
    int getWidth() const;

    int getHeight() const;
    ~CamWorker();
signals:
    void yuvFrameReady(uchar *frame, uint width, uint height);
public slots:
    void camRun();
    void camStartCapture();
    void camStopCapture();


private:
    int v4l2_fd = -1;
    int width = -1;
    int height = -1;

    void camInit();
    int camInitBuffer();



};

#endif // CAMWORKER_H
