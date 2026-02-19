#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QThread>
#include <type.h>
#include "dialog.h"
class CamWorker;
class InputFromRTSP;
class RGAWorker;
class Dialog;
class StreamInfo;
class InputManager : public QObject
{
    Q_OBJECT
public:
    explicit InputManager(QObject *parent = nullptr,
                          InputStreamType type = InputStreamType::LOCAL,
                          QString url = "",
                          RGAWorker *RGA = nullptr
            );
    ~InputManager();
signals:
    void inputStreamModeChanged();
    void processFail();
    void displayStreamInfoReady(QString streamTypem, QString url);
    void requestClear();

public slots:
     void disconn();

     void setInputStream(InputStreamType type, QString streamType, QString url, QWidget *p);
     void setInputMode(InputStreamType inputType, QString rtspURL = "");
     void dialogSuccessProcess();
     void dialogFailProcess();

private:

    InputStreamType type = InputStreamType::LOCAL;
    InputStreamType lastType;
    QString url = "";


    QThread *workT = nullptr;
    InputFromRTSP *rtspWorker = nullptr;
    CamWorker *camWorker = nullptr;
    RGAWorker *RGA = nullptr;
    Dialog *dialog = nullptr;
    StreamInfo *streamInfo = nullptr;
    void releaseThread();
    void showStreamInfo(InputStreamType type, QString streamType, QString url, QWidget *p);
private slots:



};

#endif // INPUTMANAGER_H
