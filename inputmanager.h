#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
class CamWorker;
class InputFromRTSP;
enum class  InputStreamType {
    LOCAL,
    RTSP,
};
class InputManager : public QObject
{
    Q_OBJECT
public:
    explicit InputManager(QObject *parent = nullptr, QThread *workThread = nullptr,
                          InputStreamType type = InputStreamType::LOCAL,
                          QString url = ""
            );
    ~InputManager();
signals:
    void inputStreamModeChanged();
private:
    QThread *workThread = nullptr;
    InputStreamType type = InputStreamType::LOCAL;
    InputStreamType lastType;
    QTimer timer;
    CamWorker *camWorker = nullptr;
    InputFromRTSP *rtspWorker = nullptr;
    QString url = "";
    void releaseCurStream();
    void setInputStreamMode();
private slots:
    void checkInputStreamType();
    void switchInputStream();


};

#endif // INPUTMANAGER_H
