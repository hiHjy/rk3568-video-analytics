QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

SOURCES += \
    camworker.cpp \
    main.cpp \
    rgaworker.cpp \
    widget.cpp

HEADERS += \
    camworker.h \
    rgaworker.h \
    widget.h

FORMS += \
    widget.ui


# ================== RK3568 Buildroot SDK sysroot ==================
SYSROOT = /home/alientek/rk3568_linux5.10_sdk/buildroot/output/rockchip_atk_dlrk3568/host/aarch64-buildroot-linux-gnu/sysroot

# 强制走 sysroot，避免混入 Ubuntu 本机头文件/库（交叉编译必备）
QMAKE_CFLAGS   += --sysroot=$$SYSROOT
QMAKE_CXXFLAGS += --sysroot=$$SYSROOT
QMAKE_LFLAGS   += --sysroot=$$SYSROOT

# 头文件（OpenCV/FFmpeg/RK）
INCLUDEPATH += \
    $$SYSROOT/usr/include \
    $$SYSROOT/usr/include/opencv4 \
    $$SYSROOT/usr/include/rockchip \
    $$SYSROOT/usr/include/rga \
    $$SYSROOT/usr/include/rkaiq

# 库搜索路径
LIBS += -L$$SYSROOT/usr/lib

# ------------------ OpenCV ------------------
LIBS += \
    -lopencv_core \
    -lopencv_imgproc \
    -lopencv_imgcodecs \
    -lopencv_highgui \
    -lopencv_videoio

# ------------------ FFmpeg ------------------
LIBS += \
    -lavformat \
    -lavcodec \
    -lavutil \
    -lswscale \
    -lswresample \
    -lavdevice

# ------------------ Rockchip 硬解/硬编/图像处理（常见） ------------------
# 1) MPP：RK 的解码/编码核心（你 sysroot 里有 librockchip_mpp.so）
#LIBS += -lrockchip_mpp

 #2) RGA：2D 加速/色彩空间转换/缩放（你 sysroot 里有 librga.so）
LIBS += -lrga

# 3) VPU（有些 SDK 还会用到，存在就加，不存在就删）
# 你 sysroot 里看到 librockchip_vpu.so 就保留
#LIBS += -lrockchip_vpu

# 4) rkaiq：ISP/3A（一般是相机链路才用，纯解码可不加；你有就先放着）
# 如果你项目暂时不碰 ISP，可先注释掉，少背依赖
# LIBS += -lrkaiq

# 基础依赖（很多库会间接需要）
#LIBS += -lpthread -ldl -lm -lz


# ================== Deploy（保持你原来的） ==================
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
