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
#include <QTimer>
#include <sysinfoquery.h>
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




    ui->setupUi(this);

    QListView * listView = new QListView(ui->comboBox);
    ui->comboBox->setView(listView);

    QListView * listView1 = new QListView(ui->comboBox_2);
    ui->comboBox_2->setView(listView1);

    ui->lb_remote_err->hide();
    ui->btn_local->clicked(true);

    ui->btn_remote_push_cfg->setEnabled(false);
    ui->btn_remote_push_stop->setEnabled(false);
    ui->btn_remote_push_cfg->setStyleSheet("color:grey");
    ui->btn_remote_push_stop->setStyleSheet("color:grey");

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

    qDebug() << "主线程:" << QThread::currentThread();
    sysInfo = new SysInfoQuery();
    sysInfoDisplayT = new QThread(this);
    sysInfo->moveToThread(sysInfoDisplayT);
    connect(sysInfo, &SysInfoQuery::SysInfoReady, this, &Widget::displaySysInfo);
    connect(sysInfoDisplayT, &QThread::started, sysInfo, &SysInfoQuery::tiemrStart);
    connect(sysInfoDisplayT, &QThread::finished, sysInfo, &QObject::deleteLater);

    sysInfoDisplayT->start();

    connect(this, &Widget::remotePushRequest, MPP, &MPPWorker::pushReConfig);

    connect(MPP, &MPPWorker::remotePushIsRuning, this, [this](bool flag) {
        if (flag) {
            ui->lb_remote_status->setText("正在运行");
            ui->lb_remote_status->setStyleSheet("color:green");

        } else {
            if (getIPSuccess) {
                 ui->lb_remote_status->setText("已停止");
                 ui->lb_remote_status->setStyleSheet("color:red");
                 ui->lb_remote_err->show();
            } else {

                 ui->lb_remote_status->setText("网络异常");
                 ui->lb_remote_status->setStyleSheet("color:red");
            }


            //            ui->btn_remote_push_cfg->setEnabled(true);
            //            ui->btn_remote_push_cfg->setStyleSheet("color:white");
            //            ui->btn_remote_push_stop->setEnabled(false);
            //            ui->btn_remote_push_stop->setStyleSheet("color:grey");
        }

    });

    connect(inputManager, &InputManager::displayStreamInfoReady, this, [this]() {
        ui->btn_remote_push_cfg->setEnabled(true);

        ui->btn_remote_push_cfg->setStyleSheet("color:white");

    });
}


Widget::~Widget()
{

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

    if (sysInfoDisplayT && sysInfoDisplayT ->isRunning()) {


        sysInfoDisplayT ->quit();
        if (sysInfoDisplayT ->wait(3000)) {
            qDebug() << "sysInfoDisplayT 线程正常结束";
        } else {
            qDebug() << "sysInfoDisplayT ，没有正常结束";
        }
    }
    delete ui;
}


//todo 切换输入源残影待解决

void Widget::localDisplay(char *displayFramePtr, int width, int height, uint64_t inputNum)
{
    //static int n = 0;
    if (!enableDisplay) {
        //        qDebug() << "残余的帧 flag:" << ++n;
        return;
    }

    if (inputNum != this->inputNum) {
        //        qDebug() << "inputNum fliter:" << ++n;
        return;
    }



    QImage img((uchar*)displayFramePtr, width, height, QImage::Format_RGB888);
    ui->lb_img->setPixmap(QPixmap::fromImage(img));

}



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

    /*
    enableDisplay = true;*/
    emit selectInputStream(InputStreamType::LOCAL, "本地", "", ui->widget_input_select , ++inputNum);



    enableDisplay = true;

}



void Widget::on_btn_remote_conn_clicked()
{


    QString protocol = ui->comboBox->currentText();
    QString url = "";

    if (protocol == "RTSP") {

        url = "rtsp://" + ui->le_addr->text().trimmed();
        qDebug() << "url:" << url;



        //emit selectInputStream(InputStreamType::RTSP, url);
        //QThread::msleep(20);

        emit selectInputStream(InputStreamType::RTSP, "远程", url, ui->widget_input_select, ++inputNum);


    } else if (protocol == "RTMP") {
        //todo

    }


    enableDisplay = true;




}

void Widget::displaySysInfo(QString cpuUsage, QString memUsage, QString npuUsage, QString ip, bool ipv4GetSuccess)
{
    ui->lb_cpuUsage->setText(cpuUsage + "%/400%");
    ui->lb_memUsage->setText(memUsage + "%/100%");
    ui->lb_npuUsage->setText(npuUsage + "%/100%");
    if (ipv4GetSuccess) {

        getIPSuccess = true;
        ui->lb_ip->setText(ip);


    }  else{
        getIPSuccess = false;
        ui->lb_ip->setText("网络异常,未获取到有效IP");
    }

}





void Widget::on_btn_remote_push_cfg_clicked()
{
    QString protocol = ui->comboBox_2->currentText();
    QString url = "";

    ui->btn_remote_push_cfg->setEnabled(false);
    ui->btn_remote_push_cfg->setStyleSheet("color:grey");
    ui->btn_remote_push_stop->setEnabled(true);
    ui->btn_remote_push_stop->setStyleSheet("color:white");
    if (protocol == "RTSP") {

        url = "rtsp://" + ui->le_addr_push->text().trimmed();
        qDebug() << "url:" << url;
        MPP->setUrl(url);
        emit remotePushRequest();
        QTimer::singleShot(0, this, [this]{
            emit remotePushRequest();
        });

        QTimer::singleShot(200, this, [this]{
            RGA->setRtspPushOn(true);
        });





    } else if (protocol == "RTMP") {
        //todo

    }

}




void Widget::on_btn_remote_push_stop_clicked()
{
    RGA->setRtspPushOn(false);
    ui->lb_remote_status->setText("已停止");
    ui->lb_remote_status->setStyleSheet("color:red");
    ui->lb_remote_err->hide();
    ui->btn_remote_push_cfg->setEnabled(true);
    ui->btn_remote_push_cfg->setStyleSheet("color:white");
    ui->btn_remote_push_stop->setEnabled(false);
    ui->btn_remote_push_stop->setStyleSheet("color:grey");
    qDebug() << "停止推流";
}

