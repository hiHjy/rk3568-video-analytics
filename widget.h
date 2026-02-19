 #ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "im2d.h"
#include "RockchipRga.h"
#include <im2d.hpp>
#include <dlfcn.h>
#include <rga.h>
#include "RgaUtils.h"
#include <camworker.h>
#include <QDateTime>
#include <QButtonGroup>
#include <QThread>
class MPPWorker;
class RGAWorker;
class YOLOWorker;
class InputFromRTSP;
class StreamInfo;
class Dialog;
#include <opencv2/opencv.hpp>
extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>

}

QT_BEGIN_NAMESPACE


namespace Ui { class Widget; }
QT_END_NAMESPACE


class Worker :public QThread
{
    Q_OBJECT
public:
    Worker(QObject *parent = nullptr);
    ~Worker();
    void run() override;
    static void print_error(const char *msg, int err);


};

class Widget : public QWidget
{
    Q_OBJECT

public:
    friend Worker;
    static Widget* getInstance();
    Widget(QWidget *parent = nullptr);

    Worker *worker;
    ~Widget();
signals:


    void selectInputStream(InputStreamType type, QString streamType, QString addr, QWidget *p);
public slots:

    void localDisplay(char * displayFramePtr, int width, int height);
private slots:
    void on_btn_local_clicked();

    void on_btn_remote_clicked();

    void on_btn_start_cam_clicked();

    void on_btn_remote_conn_clicked();

private:
    Ui::Widget *ui;
    QImage *img;
    bool isFirst = true;
    CamWorker* camWorker;
    InputFromRTSP *rtspWorker;
    static Widget *self;
    QButtonGroup *btnG;
    QThread *camT;
    QThread *rgaT;
    QThread *mppT;
    QThread *yoloT;
    RGAWorker *RGA;
    MPPWorker *MPP;
    YOLOWorker *YOLO;
    QDateTime date;
    char osd[128];
    uchar* encFrame;
    QWidget *streamInfo;



};


#endif // WIDGET_H
