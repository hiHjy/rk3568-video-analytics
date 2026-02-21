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
#include "postprocess.h"
#include "type.h"

extern "C" {
#include <unistd.h>

}
class RGAWorker : public QObject
{
    Q_OBJECT
public:
    explicit RGAWorker(QObject *parent = nullptr);

signals:
    void yoloRGB640X640Ready(uint8_t* yoloFrame);
    void displayFrameReady(char * displayFramePtr, int width, int height, uint64_t inputNum);
    void encFrameReady(char *encFrame, int width, int height);

public slots:
    void frameCvtColor(uchar* frame, uint32_t width, uint32_t height, InputStreamType type, uint64_t inputNum);
    void finalStep(detect_result_group_t* out);
private:
    char* RGBFrame = nullptr;
    char*  encFrame = nullptr;
    char* yoloFrame = nullptr;
    rga_buffer_t src;
    rga_buffer_t dst;
    rga_buffer_t dst_nv12;
    rga_buffer_t  yoloRGB640X640;
    uint64_t inputNum = 0;

    int count = 0;
    ~RGAWorker();

};

#endif // RGAWORKER_H
