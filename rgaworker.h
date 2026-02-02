#ifndef RGAWORKER_H
#define RGAWORKER_H

#include <QObject>
#include "im2d.h"
#include "RockchipRga.h"
#include <im2d.hpp>
#include <dlfcn.h>
#include <rga.h>
#include <memory>
#include "RgaUtils.h"

extern "C" {
#include <unistd.h>

}
class RGAWorker : public QObject
{
    Q_OBJECT
public:
    explicit RGAWorker(QObject *parent = nullptr);

signals:
    void displayFrameReady(char * displayFramePtr, int width, int height);
    void encFrameReady(char *encFrame, int width, int height);
public slots:
    void frameCvtColor(uchar* frame, uint32_t width, uint32_t height);
private:
    char* displayFrame = nullptr;
    char*  encFrame = nullptr;
    ~RGAWorker();

};

#endif // RGAWORKER_H
