#-------------------------------------------------
#
# Project created by QtCreator 2016-10-25T11:08:06
#
#-------------------------------------------------

QT       += core gui
QT       += network     \
            xml         \
            multimedia  \
            multimediawidgets \
            widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NxVideoPlayer
TEMPLATE = app

# Add Graphic Tool Library
LIBS += -L${SDKTARGETSYSROOT}/usr/lib/ -lMali  -lnxvidtex -lnx_drm_allocator -lnx_video_api 
LIBS += -L../libnxplayer/lib/32bit -lnxmpmanager -lnxfilterhelper -lnxfilter -lnxsubtitle

INCLUDEPATH += ./vr_tools/include ${SDKTARGETSYSROOT}/usr/include/ ../libnxplayer/include

SOURCES += main.cpp\
        mainwindow.cpp \
        qtglvideowindow.cpp \
        geometryengine.cpp \
        NX_CMediaPlayer.cpp \
        NX_CFileList.cpp \
        playlistwindow.cpp \
        NX_CSubtitleParser.cpp

HEADERS  += mainwindow.h \
        qtglvideowindow.h \
        geometryengine.h \
        NX_CMediaPlayer.h \
        NX_CFileList.h \
        NX_CUtil.h \
        playlistwindow.h \
        NX_CSubtitleParser.h

FORMS    += mainwindow.ui \
            playlistwindow.ui

RESOURCES += \
    shader.qrc
