QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# ========================= 调试 =========================
CONFIG += debug
QMAKE_CXXFLAGS += -g -O0
QMAKE_LFLAGS   += -g
# ========================================================

SOURCES += \
    camworker.cpp \
    inputfromrtsp.cpp \
   inputmanager.cpp \
    main.cpp \
    mppworker.cpp \
    postprocess.cpp \
    rgaworker.cpp \
    widget.cpp \
    yoloworker.cpp

HEADERS += \
    camworker.h \
    inputfromrtsp.h \
    inputmanager.h \
    mppworker.h \
    postprocess.h \
    rgaworker.h \
    type.h \
    widget.h \
    yoloworker.h

FORMS += \
    widget.ui

# ========================================================
#  RK3568 Buildroot SDK sysroot（交叉编译核心）
# ========================================================
SYSROOT = /home/alientek/rk3568_linux5.10_sdk/buildroot/output/rockchip_atk_dlrk3568/host/aarch64-buildroot-linux-gnu/sysroot

# 你新编译安装的 FFmpeg 6.1 在 sysroot 里的 /opt/ffmpeg61
#（make install DESTDIR=$$SYSROOT 时生成：$$SYSROOT/opt/ffmpeg61）
FFMPEG_PREFIX = $$SYSROOT/opt/ffmpeg61

# --------------------------------------------------------
# 强制使用 sysroot（避免混入 Ubuntu 本机头文件/库）
# --------------------------------------------------------
QMAKE_CFLAGS   += --sysroot=$$SYSROOT
QMAKE_CXXFLAGS += --sysroot=$$SYSROOT
QMAKE_LFLAGS   += --sysroot=$$SYSROOT

# ========================================================
#  Include 路径
# ========================================================
INCLUDEPATH += \
    $$SYSROOT/usr/include \
    $$SYSROOT/usr/include/opencv4 \
    $$SYSROOT/usr/include/rockchip \
    $$SYSROOT/usr/include/rga \
    $$FFMPEG_PREFIX/include

# ========================================================
#  Library 搜索路径（顺序非常关键！！！）
#  先放新 FFmpeg 的 lib，再放 sysroot/usr/lib，避免误链到旧 libav*
# ========================================================
LIBS += -L$$FFMPEG_PREFIX/lib
LIBS += -L$$SYSROOT/usr/lib

# ========================================================
#  运行时库路径（rpath）
#  让程序在板子上默认去 /opt/ffmpeg61/lib 找 libavcodec.so.60 等新库
#  （你只要把 /opt/ffmpeg61 整目录拷到板子 /opt 下即可）
# ========================================================
QMAKE_LFLAGS += -Wl,-rpath,/opt/ffmpeg61/lib

# ========================================================
#  OpenCV（你按需增减）
# ========================================================
LIBS += \
    -lopencv_core \
    -lopencv_imgproc \
    -lopencv_imgcodecs \
#    -lopencv_highgui \
#    -lopencv_videoio

# ========================================================
#  FFmpeg 6.1（来自 /opt/ffmpeg61）
#  这里名字不变，但会因为 -L 顺序优先链接到新库
# ========================================================
LIBS += \
    $$FFMPEG_PREFIX/lib/libavformat.so \
    $$FFMPEG_PREFIX/lib/libavcodec.so \
    $$FFMPEG_PREFIX/lib/libavutil.so \
    $$FFMPEG_PREFIX/lib/libswscale.so \
    $$FFMPEG_PREFIX/lib/libswresample.so \
    $$FFMPEG_PREFIX/lib/libavdevice.so \
    $$FFMPEG_PREFIX/lib/libavfilter.so \
    $$FFMPEG_PREFIX/lib/libpostproc.so
# ========================================================
#  Rockchip 硬件相关库（MPP/RGA/DRM）
#  你项目里有 mpp_enc.c，就建议把 mpp 打开
# ========================================================
LIBS += \
    -lrockchip_mpp \
    -lrga \
    -ldrm

# ========================================================
#  基础系统库（很多库间接依赖）
# ========================================================
LIBS += \
    -lpthread \
    -ldl \
    -lm \
    -lz \
    -lbz2 \
    -llzma

LIBS += \
    -lrknnrt

# ========================================================
#  Deploy（保持你原来的）
# ========================================================
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
