#include "qtglvideowindow.h"
#include "geometryengine.h"
#include <QDebug>
#include <QOpenGLTexture>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <nx_video_texture.h>
#include <drm/drm_fourcc.h>

QtGLVideoWindow::QtGLVideoWindow(QWidget *parent) : QOpenGLWidget(parent),
	m_geometries(0)
	, m_SrcStride(0)
	, m_SrcWidth(0)
	, m_SrcHeight(0)
	, m_DstWidth(0)
	, m_DstHeight(0)
	, m_pSemQueue(NULL)
	, m_pMainWindow(NULL)
	, m_seekStatus (false)
{
	int i = 0;
	m_hInYUVMem = NULL;        //  For 3D Texture Input
	m_hOutRGBMem = NULL;

	m_hMemDefault = NX_AllocateMemory( 64*64, 4 );

	for(i=0; i < MAX_CACHE_DMAFD; i++)
	{
		m_hCacheInYUVSurf[i] = NULL;
		m_cacheDmaFd[i] = 0;
	}


	m_bClearSurface = false;
}

QtGLVideoWindow::~QtGLVideoWindow()
{
	if( NULL != m_hOutRGBSurf )
	{
		nxGSurfaceDisconnectCvt2RgbaTargetToTexture(m_hOutRGBSurf);
		m_hOutRGBSurf = NULL;
	}
	nxGSurfaceDestroy();

	if( m_hMemDefault )
	{
		NX_FreeMemory(m_hMemDefault);
		m_hMemDefault = NULL;
	}

	if( m_hOutRGBMem )
	{
		NX_FreeMemory(m_hOutRGBMem);
		m_hOutRGBMem = NULL;
	}

	makeCurrent();

	if(m_geometries)
	{
		delete m_geometries;
	}

	doneCurrent();
}

void QtGLVideoWindow::initShaders()
{
	if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
		close();

	// Compile fragment shader
	if (!m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
		close();

	// Link shader pipeline
	if (!m_program.link())
		close();

	// Bind shader pipeline for use
	if (!m_program.bind())
		close();
}

void QtGLVideoWindow::initTextures()
{

}

void QtGLVideoWindow::initializeGL()
{
	initializeOpenGLFunctions();

	nxGSurfaceCreate(eglGetCurrentDisplay(), m_hMemDefault);
	nxGSurfaceSetCurrentCvt2RgbaTargetToTexture();

	glClearColor(0.f, 0.f, 0.f, 1.f);

	initShaders();
	initTextures();

	// Enable depth buffer
	glEnable(GL_DEPTH_TEST);

	// Enable back face culling
	glEnable(GL_CULL_FACE);

	m_geometries = new GeometryEngine;
}

//static int count = 0;
void QtGLVideoWindow::paintGL()
{
	GLuint textureUnit;
	int i = 0;

	m_hMutexClearSurface.Lock();
	if(m_bClearSurface)
	{
		if( NULL != m_hOutRGBSurf )
		{
			nxGSurfaceDisconnectCvt2RgbaTargetToTexture(m_hOutRGBSurf);
			nxGSurfaceSetCurrentCvt2RgbaTargetToTexture();
			m_hOutRGBSurf = NULL;
		}
		if( m_hOutRGBMem )
		{
			NX_FreeMemory(m_hOutRGBMem);
			m_hOutRGBMem = NULL;
		}
		m_hInYUVMem = NULL;

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_bClearSurface = false;

		for(i=0; i < MAX_CACHE_DMAFD; i++)
		{
			if(m_hCacheInYUVSurf[i])
			{
				nxGSurfaceDestroyCvt2RgbaSource(m_hCacheInYUVSurf[i]);
			}
			m_hCacheInYUVSurf[i] = NULL;
			m_cacheDmaFd[i] = 0;
		}

		m_hMutexClearSurface.Unlock();
		return;
	}
	else if( (false == m_bClearSurface) &&
		(NULL == m_hOutRGBMem) &&
		(m_DstWidth) &&
		(m_DstHeight) )
	{

		m_hOutRGBMem = NX_AllocateMemory( 4*m_DstWidth*m_DstHeight, 4 );
		//  Create Output Surface
		m_hOutRGBSurf = nxGSurfaceConnectCvt2RgbaTargetToTexture(m_DstWidth, m_DstHeight, m_hOutRGBMem );
	}
	m_hMutexClearSurface.Unlock();

	//  Check Input Buffer
	if( NULL == m_hInYUVMem )
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	// Clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_hInYUVSurf = NULL;
	for(i = 0; i < MAX_CACHE_DMAFD; i++)
	{
		if(m_cacheDmaFd[i] == m_hInYUVMem->dmaFd)
		{
			m_hInYUVSurf = m_hCacheInYUVSurf[i];
			break;
		}
		else
		{
			if( (0 == m_cacheDmaFd[i]) && (m_hInYUVMem))
			{
				qDebug(" DmaFd <<<<<<<  %d \n",i);
				m_cacheDmaFd[i] = m_hInYUVMem->dmaFd;
				m_hInYUVSurf = nxGSurfaceCreateCvt2RgbaSource( m_SrcStride, m_SrcWidth, m_SrcHeight, m_hInYUVMem );
				m_hCacheInYUVSurf[i] = m_hInYUVSurf;
				break;
			}
		}
	}
	if( NULL == m_hInYUVSurf)
	{
		return;
	}

	//  do CSC
	nxGSurfaceRunCvt2RgbaToTexture(m_hOutRGBSurf, m_hInYUVSurf);

	//  Get Text Nunit Number
	textureUnit = nxGSurfaceGetCvt2RgbaTextureUnit(m_hOutRGBSurf);
	nxGSurfaceSetCurrentCvt2RgbaTargetToTexture();

	if (!m_program.bind())
	{
		close();
	}

	m_program.setUniformValue("mvp_matrix", m_projection);
	m_program.setUniformValue("texture", textureUnit);

	// Draw
	m_geometries->drawCubeGeometry(&m_program);

	m_hMutexClearSurface.Lock();
	if(m_Count == 1)
	{
		m_Count--;
	}
	else
	{
		m_hMutexClearSurface.Unlock();
		return;
	}
	m_hMutexClearSurface.Unlock();
	m_pSemQueue->Post();
}

void QtGLVideoWindow::resizeGL(int /*w*/, int /*h*/)
{
	// Reset projection
	m_projection.setToIdentity();

	// Set perspective projection
	//m_projection.perspective(fov, aspect, zNear, zFar);
}


void QtGLVideoWindow::init(int SrcWidth, int SrcHeight, int DstWidth, int DstHeight)
{
	m_SrcWidth  = SrcWidth;
	m_SrcHeight = SrcHeight;
	m_DstWidth  = DstWidth;
	m_DstHeight = DstHeight;

	m_pSemQueue = new NX_CSemaphore();
	m_Count = 0;

	this->update();
}

void QtGLVideoWindow::deInit()
{
	m_hMutexClearSurface.Lock();
	m_bClearSurface = true;
	m_hMutexClearSurface.Unlock();

	if(m_pSemQueue)
	{
		m_pSemQueue->ResetSignal();
		delete m_pSemQueue;
		m_pSemQueue = NULL;
	}
	m_Count = 0;

	this->update();
}

//
//  Player Control Interfaces
//
void QtGLVideoWindow::mouseReleaseEvent(QMouseEvent */*event*/)
{
	m_pMainWindow->displayTouchEvent();
}

void QtGLVideoWindow::inputMapping( void *pData )
{
	NX_VID_MEMORY_HANDLE h2DMem = (NX_VID_MEMORY_HANDLE)pData;

	m_hMutexClearSurface.Lock();
	if( (m_bClearSurface) || (m_seekStatus))
	{
		int i = 0;
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for(i=0; i < MAX_CACHE_DMAFD; i++)
		{
			if(m_hCacheInYUVSurf[i])
			{
				nxGSurfaceDestroyCvt2RgbaSource(m_hCacheInYUVSurf[i]);
			}
			m_hCacheInYUVSurf[i] = NULL;
			m_cacheDmaFd[i] = 0;
		}
		m_hMutexClearSurface.Unlock();
		return;
	}
	m_hMutexClearSurface.Unlock();

	m_hInYUVMem = &m_hInYUVMemInfo;
	memset( &m_hInYUVMemInfo, 0, sizeof(m_hInYUVMemInfo) );
	m_hInYUVMem->drmFd   = h2DMem->drmFd;
	m_hInYUVMem->dmaFd   = h2DMem->dmaFd[0];
	m_hInYUVMem->gemFd   = h2DMem->gemFd[0];
	m_hInYUVMem->flink   = h2DMem->flink[0];
	m_hInYUVMem->size    = h2DMem->size[0];
	m_hInYUVMem->align   = h2DMem->align;
	m_SrcStride = h2DMem->stride[0];

	m_hMutexClearSurface.Lock();
	if(m_Count == 0)
	{
		m_Count++;
	}
	else
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_hMutexClearSurface.Unlock();
		return;
	}
	m_hMutexClearSurface.Unlock();
	this->update();

	if(m_Count != 1)
	{
		qDebug("&&&&&&&&&&&&&&&& m_pSemQueue->Pend(), m_Count = %d \n", m_Count);
	}

	m_pSemQueue->Pend();
}

void QtGLVideoWindow::setMainWindow(MainWindow *pMain)
{
	m_pMainWindow = pMain;
}

void QtGLVideoWindow::setSeekStatus(bool seekStatus)
{
	m_hMutexClearSurface.Lock();
	m_seekStatus = seekStatus;
	if(true == m_seekStatus)
	{
		if(m_pSemQueue)
		{
			m_pSemQueue->ResetSignal();
			m_pSemQueue->ResetValue();
			m_Count = 0;
		}

		for(int i=0; i < MAX_CACHE_DMAFD; i++)
		{
			if(m_hCacheInYUVSurf[i])
			{
				nxGSurfaceDestroyCvt2RgbaSource(m_hCacheInYUVSurf[i]);
			}
			m_hCacheInYUVSurf[i] = NULL;
			m_cacheDmaFd[i] = 0;
		}

	}
	m_hMutexClearSurface.Unlock();
}
