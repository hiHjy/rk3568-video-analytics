#include "inputmanager.h"
#include "camworker.h"
#include "inputfromrtsp.h"
#include "QDebug"
#include "streaminfo.h"
#include "rgaworker.h"
#include <QTimer>
InputManager::InputManager(QObject *parent, InputStreamType type, QString url, RGAWorker *RGA) :
    QObject(parent), type(type), url(url), RGA(RGA)
{
        lastType = type;

    //    setInputMode(type, url);


}

InputManager::~InputManager()
{

}

void InputManager::disconn()
{
    releaseThread();

}



void InputManager::showStreamInfo(InputStreamType type, QString streamType, QString url, QWidget *p)
{

    if (!streamInfo) {
        streamInfo = new StreamInfo(p);
        streamInfo->setAttribute(Qt::WA_StyledBackground, true);
        streamInfo->setAutoFillBackground(true);
        connect(this, &InputManager::displayStreamInfoReady, streamInfo, &StreamInfo::display);
        connect(streamInfo, &StreamInfo::requsetDisconn, this, [this](){
            releaseThread();

            streamInfo->hide();
            QTimer::singleShot(200, this, [this](){
                qDebug() << "执行";
                emit requestClear();


            });

        });


    }



    streamInfo->raise();

    if (type == InputStreamType::RTSP) {

        emit displayStreamInfoReady(streamType, url);
    } else if (type == InputStreamType::LOCAL) {
        url = "-";
        emit displayStreamInfoReady(streamType, url);
    }
    streamInfo->show();


}

void InputManager::setInputStream(InputStreamType type, QString streamType, QString url, QWidget *p, uint64_t inputNum)
{
    dialog = new Dialog();

    //先画对话框
    dialog->show();

    if (type == InputStreamType::RTSP) {

        lastType = type;
        //20ms后执行
        QTimer::singleShot(20, this, [=](){


            setInputMode(type, url, inputNum);

            connect(rtspWorker, &InputFromRTSP::connectSuccess, this, [=](){
                //dialog->close();

                dialog->deleteLater();
                dialog = nullptr;
                qDebug() << "执行";
                showStreamInfo(type, streamType, url, p);

            });

            connect(rtspWorker, &InputFromRTSP::reqInitDialog, this, [this](QString msg){
                if (!dialog) {
                    dialog = new Dialog();
                }

                dialog->setText(msg);
                dialog->show();


            });

            connect(rtspWorker, &InputFromRTSP::reqConnDialog, this, [this](QString msg){
                if (!dialog) {
                    dialog = new Dialog();
                }
                dialog->setText(msg);
                dialog->show();
                qDebug() << "adslfj;ladsf;adsfl;adsf";

            });
            connect(dialog, &Dialog::requestClear, this, [this] {
                if (streamInfo) {
                    streamInfo->hide();
                }
                releaseThread();
                emit requestClear();

            });
            connect(dialog, &QObject::destroyed, this, [this](){dialog = nullptr;});




        });




    } else if (type == InputStreamType::LOCAL){
        //20ms后执行
        QTimer::singleShot(20, this, [=](){


            setInputMode(type, url, inputNum);

            connect(camWorker, &CamWorker::openCamSuccess, dialog, [=](){
                //dialog->close();
                dialog->deleteLater();
                dialog = nullptr;
                qDebug() << "执行";
                showStreamInfo(type, streamType, url, p);

            });


        });


    }

}

/*
    切换输入流的模式

*/
void InputManager::setInputMode(InputStreamType inputType, QString rtspURL, uint64_t inputNum)
{

    switch (inputType) {

    case InputStreamType::LOCAL:

        releaseThread();

        workT = new QThread(this);

        camWorker = new CamWorker();
        camWorker->setInputNum(inputNum);
        camWorker->moveToThread(workT);

        connect(workT, &QThread::started, camWorker, &CamWorker::camRun);
        connect(workT, &QThread::finished, camWorker, &QObject::deleteLater);

        connect(camWorker, &CamWorker::yuvFrameReady,RGA, &RGAWorker::frameCvtColor, Qt::QueuedConnection);
        connect(camWorker,  &QObject::destroyed, this, [this](){ camWorker  = nullptr; });

        workT->start();
        qDebug() << "当前使用输入流：本地摄像头";

        break;

    case InputStreamType::RTSP:
        if (rtspURL.isEmpty()) {
            qDebug() << "url is null";
            return;
        }

        releaseThread();
        workT = new QThread(this);
        rtspWorker = new InputFromRTSP(nullptr, rtspURL);

        rtspWorker->setInputNum(inputNum);





        rtspWorker->moveToThread(workT);
        connect(workT, &QThread::started, rtspWorker, &InputFromRTSP::decodeH264ToNV12);
        connect(rtspWorker, &InputFromRTSP::yuvFrameReady, RGA, &RGAWorker::frameCvtColor, Qt::QueuedConnection);
        connect(workT, &QThread::finished, rtspWorker, &QObject::deleteLater);
        connect(rtspWorker, &QObject::destroyed, this, [this](){ rtspWorker = nullptr; });

        workT->start();
        qDebug() << "当前使用输入流：rtsp";
        break;
    }
}








void InputManager::releaseThread()
{
    if (workT && workT->isRunning()) {
        if (lastType == InputStreamType::RTSP && rtspWorker) {
            rtspWorker->setStop_flag(true);
        }
        workT->requestInterruption();
        workT->quit();
        if (workT->wait(5000)) {

            qDebug() << "输入流线程已经释放";
        } else {
            qDebug() << "输入流线程，没有正常结束，切换输入流失败";
            exit(-1);
        }

        workT = nullptr;

    }

}




