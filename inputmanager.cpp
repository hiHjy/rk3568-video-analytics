#include "inputmanager.h"
#include "camworker.h"
#include "inputfromrtsp.h"
#include "QDebug"
#include "rgaworker.h"
InputManager::InputManager(QObject *parent, InputStreamType type, QString url, RGAWorker *RGA) :
    QObject(parent), type(type), url(url), RGA(RGA)
{
    lastType = type;

    setInputMode(type, url);


}

InputManager::~InputManager()
{

}

/*
    切换输入流的模式

*/
void InputManager::setInputMode(InputStreamType inputType, QString rtspURL)
{

    switch (inputType) {

    case InputStreamType::LOCAL:

        releaseThread();

        workT = new QThread(this);

        camWorker = new CamWorker();

        camWorker->moveToThread(workT);

        connect(workT, &QThread::started, camWorker, &CamWorker::camRun);
        connect(workT, &QThread::finished, camWorker, &QObject::deleteLater);

        connect(camWorker, &CamWorker::yuvFrameReady,RGA, &RGAWorker::frameCvtColor, Qt::QueuedConnection);


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
        rtspWorker->moveToThread(workT);
        connect(workT, &QThread::started, rtspWorker, &InputFromRTSP::decodeH264ToNV12);
        connect(rtspWorker, &InputFromRTSP::yuvFrameReady, RGA, &RGAWorker::frameCvtColor, Qt::QueuedConnection);
        connect(workT, &QThread::finished, rtspWorker, &QObject::deleteLater);
        workT->start();
        qDebug() << "当前使用输入流：rtsp";
        break;
    }
}

void InputManager::releaseThread()
{
    if (workT && workT->isRunning()) {

        workT->requestInterruption();
        workT->quit();
        if (workT->wait(2000)) {
            qDebug() << "输入流线程已经释放";
        } else {
            qDebug() << "输入流线程，没有正常结束，切换输入流失败";
            exit(-1);
        }

        workT = nullptr;

    }

}




