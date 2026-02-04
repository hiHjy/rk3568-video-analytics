#ifndef YOLOWORKER_H
#define YOLOWORKER_H

extern "C" {

#include <dlfcn.h>
// 标准输入输出头文件
#include <stdio.h>
// 标准库（内存分配、程序退出等）
#include <stdlib.h>
// 字符串操作头文件
#include <string.h>
// 时间相关头文件（用于统计推理耗时）
#include <sys/time.h>
#include "rknn/rknn_api.h"
}

#include "RgaUtils.h"
// RGA图像操作核心头文件
#include "im2d.h"
// OpenCV核心模块（图像数据结构、基本操作）
#include "opencv2/core/core.hpp"
// OpenCV图像编解码模块（读取、保存图片）
#include "opencv2/imgcodecs.hpp"
// OpenCV图像处理模块（色彩空间转换等）
#include "opencv2/imgproc.hpp"
// 目标检测后处理头文件（NMS、边界框解析等）
#include "postprocess.h"
// RGA核心API头文件
#include "rga.h"
// RKNN核心API头文件（模型加载、推理执行等）

#include <QObject>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

#define PERF_WITH_POST 1 // 定义是否在性能测试中包含后处理步骤（1=包含，0=不包含）

class YOLOWorker : public QObject
{
    Q_OBJECT
public:
    explicit YOLOWorker(QObject *parent = nullptr);

signals:
    // 你如果后面要跨线程把结果发出去，可以加信号
    // void yoloResultReady(detect_result_group_t result);

public slots:
    void inferRgb640(uint8_t* rgb);

private:
    uint64_t t_in = 0;
    /*
     * @brief 打印RKNN张量（tensor）的属性信息，用于调试和查看模型输入输出参数
     * @param attr 指向rknn_tensor_attr结构体的指针，包含张量的索引、名称、形状、类型等信息
     */
    static void dump_tensor_attr(rknn_tensor_attr* attr);

    /*
     * @brief 将timeval结构体转换为微秒（us）单位的浮点值，用于计算时间差
     * @param t timeval结构体，包含秒（tv_sec）和微秒（tv_usec）两个成员
     * @return 转换后的总微秒数（浮点型，保留精度）
     */
    static double __get_us(struct timeval t);

    /*
     * @brief 加载RKNN模型文件到内存中，获取模型数据和模型大小
     * @param filename RKNN模型文件路径（如xxx.rknn）
     * @param model_size 输出参数，用于存储模型文件的总字节大小
     * @return 成功：指向模型数据的内存缓冲区指针；失败：NULL
     */
    static unsigned char* load_model(const char* filename, int* model_size);

    /*
     * @brief 从指定文件中读取指定偏移量和大小的数据，加载到内存中
     * @param fp 文件指针（已打开的二进制文件）
     * @param ofst 读取数据的起始偏移量（相对于文件开头）
     * @param sz 要读取的数据字节大小
     * @return 成功：指向读取数据的内存缓冲区指针；失败：NULL
     */
    static unsigned char* load_data(FILE* fp, size_t ofst, size_t sz);

    /*
     * @brief 将浮点型数据保存到文本文件中，用于调试（查看模型输出结果）
     * @param file_name 输出文本文件路径
     * @param output 浮点型数据缓冲区指针
     * @param element_size 浮点型数据的元素个数
     * @return 成功：0；失败：-1（此处简化处理，默认返回0）
     */
    static int saveFloat(const char* file_name, float* output, int element_size);

    // ----------------- 你原来的配置保持不动 -----------------
    const char* model_name = "yolov5s-640-640.rknn";
    const float nms_threshold      = NMS_THRESH;
    const float box_conf_threshold = BOX_THRESH;

    int model_data_size = 0;
    rknn_context ctx = 0;
    rknn_input_output_num io_num;
    unsigned char* model_data = nullptr;

    // ----------------- 新增：把 demo 里 init 后需要长期保存的东西做成成员 -----------------
    int model_w = 0;
    int model_h = 0;
    int model_c = 0;

    // 输入/输出属性（后处理需要 out_scales/out_zps）
    std::vector<rknn_tensor_attr> output_attrs;
    std::vector<float> out_scales;
    std::vector<int32_t> out_zps;

    // IO buffer 描述（每帧复用，避免反复构造）
    rknn_input inputs[1];                 // yolov5 demo 一般 1 input
    std::vector<rknn_output> outputs;     // yolov5 demo 一般 3 output
    detect_result_group_t* out;
    int initYOLO();

    ~YOLOWorker();

    // 禁止拷贝（防止 ctx/model_data 被浅拷贝双 free）
    YOLOWorker(const YOLOWorker&) = delete;
    YOLOWorker& operator=(const YOLOWorker&) = delete;
};

#endif // YOLOWORKER_H
