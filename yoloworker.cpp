#include "yoloworker.h"
#include "postprocess.h"
#include <QDebug>
#include <opencv2/opencv.hpp>
YOLOWorker::YOLOWorker(QObject *parent) : QObject(parent)
{
    initYOLO();




}

void YOLOWorker::drawRect(uint8_t* rgb)
{
    cv::Mat img(640, 640, CV_8UC3, rgb);



    if (!out || out->count <= 0) {
        emit drawRectFinish();
        return;
    }


       for (int i = 0; i < out->count; ++i) {
           detect_result_t* det = &out->results[i];

           int x1 = det->box.left;
           int y1 = det->box.top;
           int x2 = det->box.right;
           int y2 = det->box.bottom;

           // 1️⃣ 画矩形框
           cv::rectangle(
               img,
               cv::Point(x1, y1),
               cv::Point(x2, y2),
               cv::Scalar(0, 255, 0),   // 绿色
               2
           );

           // 2️⃣ 画文字（类别 + 置信度）
           char label[128];
           snprintf(label, sizeof(label), "%s %.1f%%",
                    det->name, det->prop * 100);

           cv::putText(
               img,
               label,
               cv::Point(x1, y1 - 5),
               cv::FONT_HERSHEY_SIMPLEX,
               0.5,
               cv::Scalar(0, 255, 0),
               1
           );
       }
       emit drawRectFinish();

}

void YOLOWorker::inferRgb640(uint8_t *rgb)
{


    // 你传进来的 rgb 就是 RGA 那边处理好的 RGB888 640x640
    // 注意：必须保证内存布局是紧密的（stride == 640*3），否则需要你在 RGA 输出时就做成紧密buffer


    if (!rgb) {
        printf("[inferRgb640] rgb is null!\n");
        return;
    }
    if (!out) {
        printf("[inferRgb640] out is null!\n");
        return;
    }
    if (!ctx) {
        printf("[inferRgb640] ctx not initialized!\n");
        return;
    }
    if (io_num.n_output <= 0) {
        printf("[inferRgb640] invalid io_num.n_output=%d\n", io_num.n_output);
        return;
    }

    int ret = -1;

    // 输入就是模型尺寸，不需要 scale 回原图（你只喂 640）
    float scale_w = 1.0f;
    float scale_h = 1.0f;

    // 你真正要喂给 rknn 的就是 rgb
    inputs[0].buf = rgb;

    timeval t0, t1, t2;
    gettimeofday(&t0, NULL);
    printf("间隔=%.2fms\n", __get_us(t0) / 1000.0 - t_in/ 1000.0);
    t_in = __get_us(t0);

    ret = rknn_inputs_set(ctx, 1, inputs);
    if (ret < 0) {
        printf("rknn_inputs_set ret=%d\n", ret);
        return;
    }

    ret = rknn_run(ctx, NULL);
    if (ret < 0) {
        printf("rknn_run ret=%d\n", ret);
        return;
    }

    ret = rknn_outputs_get(ctx, io_num.n_output, outputs.data(), NULL);
    if (ret < 0) {
        printf("rknn_outputs_get ret=%d\n", ret);
        return;
    }

    gettimeofday(&t1, NULL);
    // 输入就是模型尺寸，不需要 scale 回原图（你只喂 640）

    // pads：letterbox 填充信息
    // 你现在喂的是“已经处理好的 640x640”，等价于没有 padding
    BOX_RECT pads;
    memset(&pads, 0, sizeof(pads));
    pads.bottom = 160;
    pads.top = 0;
    pads.left = 0;
    pads.right = 0;


    // 把结果写到 out（别在这里创建局部 group 然后丢了）
    memset(out, 0, sizeof(detect_result_group_t));

    // 注意：yolov5 demo 是 3 个输出；如果你模型不是 3 个输出，这里要适配
    // 你前面跑过：input=1 output=3，所以这里按 3 写
    if (io_num.n_output >= 3) {
        post_process(
                    (int8_t*)outputs[0].buf,
                (int8_t*)outputs[1].buf,
                (int8_t*)outputs[2].buf,

                model_h, model_w,
                box_conf_threshold, nms_threshold,
                pads,
                scale_w, scale_h,
                out_zps, out_scales,
                out
                );
    } else {
        printf("[inferRgb640] unexpected n_output=%d (yolov5 usually 3)\n", io_num.n_output);
    }

    gettimeofday(&t2, NULL);

    printf("run=%.2f ms, post=%.2f ms, det=%d\n",
           (__get_us(t1)-__get_us(t0))/1000.0,
           (__get_us(t2)-__get_us(t1))/1000.0,
           out->count);
    drawRect(rgb);
    // 释放输出（非常关键）
    rknn_outputs_release(ctx, io_num.n_output, outputs.data());
    qDebug() << "done";
}

void YOLOWorker::dump_tensor_attr(rknn_tensor_attr *attr)
{
    // 拼接张量形状字符串（如 [1, 3, 640, 640]）
    std::string shape_str = attr->n_dims < 1 ? "" : std::to_string(attr->dims[0]);
    for (int i = 1; i < attr->n_dims; ++i) {
        shape_str += ", " + std::to_string(attr->dims[i]);
    }

    // 打印张量的完整属性信息
    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, w_stride = %d, size_with_stride=%d, fmt=%s, "
           "type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index,        // 张量索引（输入/输出的序号）
           attr->name,         // 张量名称
           attr->n_dims,       // 张量维度数（如4维：NCHW/NHWC）
           shape_str.c_str(),  // 张量形状字符串
           attr->n_elems,      // 张量元素总个数
           attr->size,         // 张量数据字节大小
           attr->w_stride,     // 张量宽度方向步长（用于对齐）
           attr->size_with_stride,  // 考虑步长后的张量总字节大小
           get_format_string(attr->fmt),  // 张量格式字符串（如NCHW/NHWC，通过工具函数转换）
           get_type_string(attr->type),   // 张量数据类型字符串（如UINT8/FLOAT32，通过工具函数转换）
           get_qnt_type_string(attr->qnt_type),  // 张量量化类型字符串（如对称量化/非对称量化，通过工具函数转换）
           attr->zp,           // 量化零点（仅量化张量有效）
           attr->scale);       // 量化缩放因子（仅量化张量有效）
}

double YOLOWorker::__get_us(timeval t)
{
    return (t.tv_sec * 1000000 + t.tv_usec);
}

unsigned char *YOLOWorker::load_model(const char *filename, int *model_size)
{
    FILE*          fp;
    unsigned char* data;

    // 以二进制只读模式打开模型文件
    fp = fopen(filename, "rb");
    if (NULL == fp) {
        printf("Open file %s failed.\n", filename);  // 文件打开失败
        return NULL;
    }

    // 将文件指针移动到文件末尾，用于获取文件总大小
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);  // 获取当前文件指针位置（即文件总字节数）

    // 从文件开头（偏移量0）读取整个文件的数据到内存
    data = load_data(fp, 0, size);

    // 关闭文件指针，释放文件资源
    fclose(fp);

    // 输出模型大小到调用者
    *model_size = size;

    // 返回模型数据缓冲区指针
    return data;
}

unsigned char *YOLOWorker::load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char* data;
    int            ret;

    data = NULL;

    // 检查文件指针是否有效
    if (NULL == fp) {
        return NULL;
    }

    // 将文件指针移动到指定偏移量位置
    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0) {
        printf("blob seek failure.\n");  // 文件偏移失败
        return NULL;
    }

    // 分配内存缓冲区，用于存储读取的数据
    data = (unsigned char*)malloc(sz);
    if (data == NULL) {
        printf("buffer malloc failure.\n");  // 内存分配失败
        return NULL;
    }

    // 从文件中读取sz字节的数据到内存缓冲区
    ret = fread(data, 1, sz, fp);
    (void)ret; // demo里不严格校验读取字节数，这里避免 unused warning

    // 返回数据缓冲区指针（即使读取字节数不足，也返回已分配的内存，由调用者处理）
    return data;
}

int YOLOWorker::saveFloat(const char *file_name, float *output, int element_size)
{
    FILE* fp;
    // 以写入模式打开文本文件（不存在则创建，存在则覆盖）
    fp = fopen(file_name, "w");

    // 循环写入每个浮点元素，保留6位小数
    for (int i = 0; i < element_size; i++) {
        fprintf(fp, "%.6f\n", output[i]);
    }

    // 关闭文件指针
    fclose(fp);

    return 0;
}

int YOLOWorker::initYOLO()
{


    // 这俩是成员：model_data_size / model_data
    model_data = load_model(model_name, &model_data_size);
    if (!model_data) return -1;

    int ret = rknn_init(&ctx, model_data, model_data_size, 0, NULL);
    if (ret < 0) {
        printf("rknn_init ret=%d\n", ret);
        return -1;
    }

    memset(&io_num, 0, sizeof(io_num));
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0) {
        printf("query io_num ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // ---- 2) query input dims ----
    rknn_tensor_attr input_attr;
    memset(&input_attr, 0, sizeof(input_attr));
    input_attr.index = 0;
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &input_attr, sizeof(input_attr));
    if (ret < 0) {
        printf("query input attr ret=%d\n", ret);
        return -1;
    }

    // 保存到成员：model_w/model_h/model_c（infer/post需要）
    if (input_attr.fmt == RKNN_TENSOR_NCHW) {
        model_c = input_attr.dims[1];
        model_h = input_attr.dims[2];
        model_w = input_attr.dims[3];
    } else {
        model_h = input_attr.dims[1];
        model_w = input_attr.dims[2];
        model_c = input_attr.dims[3];
    }
    printf("model input: %dx%dx%d fmt=%d\n", model_w, model_h, model_c, input_attr.fmt);

    // 你说你送的是 RGB 640x640
    if (model_w != 640 || model_h != 640 || model_c != 3) {
        printf("WARNING: model expects %dx%dx%d, but you plan to feed 640x640x3\n",
               model_w, model_h, model_c);
    }

    // ---- 3) query output attrs -> scales/zps ----
    output_attrs.resize(io_num.n_output);
    out_scales.clear();
    out_zps.clear();
    out_scales.reserve(io_num.n_output);
    out_zps.reserve(io_num.n_output);

    for (int i = 0; i < io_num.n_output; i++) {
        memset(&output_attrs[i], 0, sizeof(rknn_tensor_attr));
        output_attrs[i].index = i;
        rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr));
        out_scales.push_back(output_attrs[i].scale);
        out_zps.push_back(output_attrs[i].zp);
    }

    // ---- 4) prepare io structs ----
    // 注意：这里必须初始化“成员 inputs”，你之前写成了局部 rknn_input inputs[1]，会把成员遮蔽掉
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type  = RKNN_TENSOR_UINT8;
    inputs[0].fmt   = RKNN_TENSOR_NHWC;         // 你送 RGB888，且 demo 后处理按 NHWC 输入写的
    inputs[0].size  = model_w * model_h * model_c;
    inputs[0].pass_through = 0;

    // outputs 也是成员，init 一次，后面每帧复用
    outputs.resize(io_num.n_output);
    memset(outputs.data(), 0, sizeof(rknn_output) * io_num.n_output);
    for (int i = 0; i < io_num.n_output; i++) {
        outputs[i].index = i;
        outputs[i].want_float = 0;  // int8 输出给 post_process
        outputs[i].is_prealloc = 0;
    }

    out = (detect_result_group_t*)malloc(sizeof(detect_result_group_t));
    printf("post process config: conf=%.2f nms=%.2f\n", box_conf_threshold, nms_threshold);

    return 0;
}

YOLOWorker::~YOLOWorker()
{
    deinitPostProcess();
    if (ctx) {
        rknn_destroy(ctx);
        ctx = 0;
    }
    if (model_data) {
        free(model_data);
        model_data = nullptr;
    }
    if (out) {
        free(out);
        out = nullptr;
    }
}
