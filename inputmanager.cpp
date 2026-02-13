#include "inputmanager.h"
#include "camworker.h"
#include "inputfromrtsp.h"
#include "QDebug"
InputManager::InputManager(QObject *parent, QThread *workThread, InputStreamType type, QString url) :
    QObject(parent), workThread(workThread), type(type), url(url)
{
    lastType = type;
    setInputStreamMode();

    timer.start(100);
    connect(&timer, &QTimer::timeout, this, &InputManager::checkInputStreamType);
    connect(this, &InputManager::inputStreamModeChanged, this, &InputManager::switchInputStream);
    connect(workThread, &QThread::finished, rtspWorker, &QObject::deleteLater);
    connect(workThread, &QThread::finished, camWorker, &QObject::deleteLater);
}

InputManager::~InputManager()
{
    if (workThread && workThread->isRunning()) {

        workThread->requestInterruption();
        workThread->quit();
        if (workThread->wait(3000)) {
            qDebug() << "输入线程正常结束";
        } else {
            qDebug() << "输入线程还在运行，没有正常结束";
        }
    }
}

void InputManager::releaseCurStream()
{
    //先释放资源
    switch (lastType) {

    case InputStreamType::LOCAL:
        camWorker->~CamWorker();
        camWorker = nullptr;
        break;
    case InputStreamType::RTSP:
        rtspWorker->~InputFromRTSP();
        rtspWorker = nullptr;
        break;
    }
}

void InputManager::setInputStreamMode()
{
    switch (type) {

    case InputStreamType::LOCAL:
        camWorker = new CamWorker();

        camWorker->moveToThread(workThread);
        break;
    case InputStreamType::RTSP:
        if (url.isEmpty()) {
            qDebug() << "url is empty 这里可以弹个提示框";
            return;
        }
        rtspWorker = new InputFromRTSP(nullptr, url);
        rtspWorker->moveToThread(workThread);
        break;
    }

}

void InputManager::checkInputStreamType()
{
    if (lastType != type) {
        qDebug() << "输入流模式改变";
        emit inputStreamModeChanged();
    }
}

void InputManager::switchInputStream()
{

    releaseCurStream();
    setInputStreamMode();


}


