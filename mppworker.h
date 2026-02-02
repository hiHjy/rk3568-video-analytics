#ifndef MPPWORKER_H
#define MPPWORKER_H

#include <QObject>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>

}
class MPPWorker : public QObject
{
    Q_OBJECT
public:
    explicit MPPWorker(QObject *parent = nullptr);

signals:
public slots:
    void encode2H264(char *nv12Frame, int width, int height);
private:
    void print_error(const char *msg, int err);

};

#endif // MPPWORKER_H
