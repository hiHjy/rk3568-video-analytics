#include "camworker.h"
#include <QThread>
#define V4L2_DEV_PATH "/dev/video10"
#define FRAMEBUFFER_COUNT 4
#define WIDTH 640
#define HEIGHT 480
#define FPS 30
#define FORMAT V4L2_PIX_FMT_YUYV

CamWorker::CamWorker(QObject *parent) : QObject(parent)
{
    camInit();
    camInitBuffer();
    //camRun();
    //qDebug() << QString::number(V4L2_PIX_FMT_YUYV, 16);

}

void CamWorker::camRun()
{
    struct pollfd fds;
    int ret = -1;
    fds.fd = v4l2_fd;
    fds.events = POLLIN;
    struct v4l2_buffer buf;
    uchar *tmpBuf = (uchar*)malloc(buf_infos[0].length);
    if (!tmpBuf) {
        perror("malloc");
        return;
    }
    camStartCapture();



    qDebug() << "采集线程:" << QThread::currentThread();
    while (!QThread::currentThread()->isInterruptionRequested()) {

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        ret = poll(&fds, 1, 100);
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        if (ret == 0) continue; //如果超时,那么重新来
        if (ret < 0) {
            qDebug() << "[camRun]poll error";
            break;
        }
        //如果设备错误，挂起，fd被关闭
        if (fds.revents & (POLLERR|POLLHUP|POLLNVAL)) {
            qDebug() << "poll err revents=" << fds.revents;
            break; // 或者走重连逻辑
        }

        if (fds.revents & POLLIN) {
            if (ioctl(v4l2_fd, VIDIOC_DQBUF, &buf) != 0) {
                qDebug() << "获取视频帧失败";
                continue;
            }
            size_t n = buf.bytesused ? buf.bytesused : buf_infos[buf.index].length;
            memcpy(tmpBuf, buf_infos[buf.index].start, n);

            //qDebug() << "emit yuvFrameReady in thread" << QThread::currentThread();
            emit yuvFrameReady(tmpBuf, width, height, InputStreamType::LOCAL);

            //获取到一帧数据
            if(ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
                perror("QBUF");
                break;
            }


        }

        if (isFirst) {
            isFirst = false;
            emit openCamSuccess();
        }

        //        qDebug() << "获取1帧数据（前五个字节）：" << buf_infos[buf.index].start[0]
        //                << buf_infos[buf.index].start[1]
        //                <<buf_infos[buf.index].start[2] <<buf_infos[buf.index].start[3] <<buf_infos[buf.index].start[4];


    }
    free(tmpBuf);
    qDebug() << "采集线程成功退出循环";
    camStopCapture();

}

void CamWorker::camInit()
{
    width = WIDTH;
    height = HEIGHT;

    struct v4l2_capability cap;
    buf_infos = (struct cam_buf*)malloc(sizeof(struct cam_buf) * FRAMEBUFFER_COUNT);
    printf("正在初始化v4l2设备...\n");

    /* 打开摄像头 */
    v4l2_fd = open(V4L2_DEV_PATH, O_RDWR);
    if (v4l2_fd < 0) {
        perror("open v4l2_dev error");
        exit(-1);
    }
    /*查询设备功能*/
    if (ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("ioctl VIDIOC_QUERYCAP error");
        close(v4l2_fd);
        exit(-1);
    }

    /*判断是否为视频采集设备*/
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        //如果不是视频采集设备 !(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities) 值为非0
        printf("非视频采集设备\n");
        close(v4l2_fd);
        exit(-1);
    }

    /*查询采集设备支持的所有像素格式及描述信息*/
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));

    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct camera_format camfmts[10];
    memset(camfmts, 0, sizeof(camfmts));

    while (ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        printf("index:%d 像素格式:0x%x, 描述信息:%s\n",
               fmtdesc.index,
               fmtdesc.pixelformat,
               fmtdesc.description);
        /*将支持的像素格式存入结构体数组*/
        strncpy(camfmts[fmtdesc.index].description, (const char*)fmtdesc.description, sizeof(fmtdesc.description));
        camfmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        fmtdesc.index++;
    }
    printf("已获取全部支持的格式\n");

    /* 枚举出摄像头所支持的所有视频采集分辨率 */

    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;

    // 方法1：分别清零
    memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
    memset(&frmival, 0, sizeof(struct v4l2_frmivalenum));

    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (int i = 0; camfmts[i].pixelformat; ++i) {

        frmsize.index = 0;
        frmsize.pixel_format = camfmts[i].pixelformat;  // 设置要查询的像素格式
        frmival.pixel_format = camfmts[i].pixelformat;  // 设置要查询帧率的像素格式

        while (ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {

            printf("[%s]size<%d*%d> ",camfmts[i].description,
                   frmsize.discrete.width,//宽
                   frmsize.discrete.height);//高
            frmsize.index++;

            /*查询帧率的像素格式*/
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {

                printf(" <%dfps> ", frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");

        }
        printf("\n");
    }

    //设置像素格式

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    struct v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));

    /* 设置帧格式 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat =  FORMAT;

    if (ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("ioctl VIDIOC_QUERYCAP error");
        close(v4l2_fd);
        return;
    }

    if (fmt.fmt.pix.pixelformat !=  FORMAT) {
        fprintf(stderr, "像素格式不支持\n");
        close(v4l2_fd);
        return;
    }

    if (ioctl(v4l2_fd, VIDIOC_G_FMT, &fmt) < 0) {
        perror("ioctl VIDIOC_QUERYCAP error");
        close(v4l2_fd);
        return;
    }

    printf("当前视频分辨率为<%d * %d> byteperline:%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline);

    /* 获取 streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm) < 0) {
        perror("ioctl VIDIOC_G_PARM error");
        close(v4l2_fd);
        return;
    }

    /*检测是否支持帧率设置*/
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        //走到这里表示支持帧率设置


        /*设置30fps*/
        printf("该v4l2设备支持帧率设置\n");
        streamparm.parm.capture.timeperframe.denominator = FPS;
        streamparm.parm.capture.timeperframe.numerator = 1;
        if (ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm) < 0) {
            perror("ioctl VIDIOC_S_PARM error");
            close(v4l2_fd);
            return;
        }

    }
    struct v4l2_format fmt_real;
    memset(&fmt_real, 0, sizeof(fmt_real));
    fmt_real.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(v4l2_fd, VIDIOC_G_FMT, &fmt_real) < 0) {
        perror("VIDIOC_G_FMT error");
    } else {
        printf("[实际分辨率格式] %d x %d\n",
               fmt_real.fmt.pix.width,
               fmt_real.fmt.pix.height);
    }

    struct v4l2_streamparm parm_real;
    memset(&parm_real, 0, sizeof(parm_real));
    parm_real.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(v4l2_fd, VIDIOC_G_PARM, &parm_real) < 0) {
        perror("VIDIOC_G_PARM error");
    } else {
        if (parm_real.parm.capture.timeperframe.numerator != 0) {
            double fps = (double)parm_real.parm.capture.timeperframe.denominator /
                    parm_real.parm.capture.timeperframe.numerator;

            printf("[实际帧率] %.2f fps\n", fps);
        }
    }

    switch (fmt_real.fmt.pix.pixelformat) {
    case V4L2_PIX_FMT_YUYV:
        printf("[实际格式] %s\n", "YUYV");
        break;
    case V4L2_PIX_FMT_MJPEG:
        printf("[实际格式] %s\n", "MJPEG");
        break;;
    }
}

int CamWorker::camInitBuffer()
{
    /*申请缓冲区*/
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    memset(&buf, 0, sizeof(buf));
    reqbuf.count = FRAMEBUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        perror("ioctl VIDIOC_REQBUFS error");
        close(v4l2_fd);
        exit(-1);
    }

    /*建立内存映射*/
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; ++buf.index) {
        if (ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0) return -1; //查询buf的信息
        buf_infos[buf.index].length = buf.length;
        buf_infos[buf.index].start = (unsigned char*)mmap(NULL,
                                                          buf.length,
                                                          PROT_READ |PROT_WRITE,
                                                          MAP_SHARED,
                                                          v4l2_fd,
                                                          buf.m.offset
                                                          );

        if (buf_infos[buf.index].start == MAP_FAILED) {
            fprintf(stderr, "index%d号缓冲区内存映射失败\n", buf.index);
            for (int i = 0; i < (int)buf.index; ++i) {
                munmap(buf_infos[i].start, buf_infos[i].length);


            }
            close(v4l2_fd);
            return -1;
        }
        printf("[USER]缓冲区%d 起始地址:%p 大小:%lu\n", buf.index, buf_infos[buf.index].start, buf_infos[buf.index].length);

    }
    printf("摄像头内核帧缓冲区已经全部映射到用户空间\n");


    //作用：将缓冲区放入摄像头的输入队列，等待摄像头填充数据
    for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; ++buf.index) {
        if (ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("ioctl VIDIOC_QBUF error");
            close(v4l2_fd);
            return -1;

        }

    }

    printf("帧缓存区已准备就绪!\n");
    return 0;
}

void CamWorker::camStartCapture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2_fd, VIDIOC_STREAMON, &type) != 0) {
        qDebug() << "启动采集失败";
        return;
    }
    qDebug() << "启动采集";

}

void CamWorker::camStopCapture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type) != 0) {
        qDebug() << "停止采集失败";
        return;
    }
    qDebug() << "停止采集";

}

int CamWorker::getHeight() const
{
    return height;
}

int CamWorker::getWidth() const
{
    return width;
}







CamWorker::~CamWorker()
{



    for (int i = 0; i < FRAMEBUFFER_COUNT; ++i) {
        munmap(buf_infos[i].start, buf_infos[i].length);
    }
    close(v4l2_fd);
    if (buf_infos) free(buf_infos);
    qDebug() << "采集工作线程实例被释放";

}
