#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QThread>
#include <type.h>
class CamWorker;
class InputFromRTSP;
class RGAWorker;
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
public slots:
     void setInputMode(InputStreamType inputType, QString rtspURL = "");
private:

    InputStreamType type = InputStreamType::LOCAL;
    InputStreamType lastType;
    QString url = "";


    QThread *workT = nullptr;
    InputFromRTSP *rtspWorker = nullptr;
    CamWorker *camWorker = nullptr;
    RGAWorker *RGA = nullptr;

    void releaseThread();

private slots:



};

#endif // INPUTMANAGER_H
