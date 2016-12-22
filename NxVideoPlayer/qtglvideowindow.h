#ifndef QTGLVIDEOWINDOW_H
#define QTGLVIDEOWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

#include <NX_MoviePlay.h>
#include <nx_video_api.h>
#include <nx_video_texture.h>

#include <mainwindow.h>

#include "NX_CUtil.h"

#define MAX_CACHE_DMAFD		30

class QOpenGLShaderProgram;
class QOpenGLTexture;
class GeometryEngine;

class QtGLVideoWindow : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
private:
    QOpenGLShaderProgram m_program;
    GeometryEngine* m_geometries;
    QMatrix4x4 m_projection;

public:
    explicit QtGLVideoWindow(QWidget *parent = 0);
    ~QtGLVideoWindow();

    void init(int SrcWidth, int SrcHeight, int DstWidth, int DstHeight);
    void deInit();

    //
    //  Player Control Interfaces
    //
    virtual void mouseReleaseEvent(QMouseEvent *event); //DisplayTouch
    void inputMapping( void *pData);                    //Video Memory Map
    void setMainWindow(MainWindow *pMain);              //MainWindow Handle
    void setSeekStatus(bool seekStatus);                  //Seek Status

private:
    int m_SrcStride;
    int m_SrcWidth;
    int m_SrcHeight;
    int m_DstWidth;
    int m_DstHeight;

    //  Default Memory
    NX_MEMORY_HANDLE    m_hMemDefault;

    //  Input Memory : CSC Result
    NX_MEMORY_HANDLE    m_hInYUVMem;        //  For 3D Texture Input
    NX_MEMORY_INFO      m_hInYUVMemInfo;
    HSURFSOURCE         m_hInYUVSurf;
    HSURFSOURCE         m_hCacheInYUVSurf[MAX_CACHE_DMAFD];
    int                 m_cacheDmaFd[MAX_CACHE_DMAFD];

    //  Output memory & Surface
    NX_MEMORY_HANDLE    m_hOutRGBMem;
    HSURFBOUNDTARGET    m_hOutRGBSurf;

    bool                m_bClearSurface;
    NX_CMutex			m_hMutexClearSurface;
    NX_CSemaphore       *m_pSemQueue;
    int m_Count;

    MainWindow			*m_pMainWindow;
    bool                m_seekStatus;

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    void initShaders();
    void initTextures();
};

#endif // QTGLVIDEOWINDOW_H
