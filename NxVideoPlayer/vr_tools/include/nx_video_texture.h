#ifndef __VR_VIDEO_TEXTURE__
#define __VR_VIDEO_TEXTURE__

//------------------------------------------------------------------------------
//
//    Includes
//    
//------------------------------------------------------------------------------
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vr_common_def.h"

#ifdef __cplusplus
extern "C" {
#endif



//------------------------------------------------------------------------------
//
//    Defines
//    
//------------------------------------------------------------------------------
typedef void* HSURFTARGET;
typedef void* HSURFSOURCE;
typedef void* HSURFBOUNDTARGET;



//------------------------------------------------------------------------------
//
//    Functions
//    
//------------------------------------------------------------------------------
EGLBoolean nxGSurfaceCreate(EGLDisplay display, NX_MEMORY_HANDLE default_vmem);
void  nxGSurfaceDestroy(void);

HSURFSOURCE nxGSurfaceCreateCvt2RgbaSource(unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, void* hData);
void        nxGSurfaceDestroyCvt2RgbaSource(HSURFSOURCE hSource);

EGLBoolean nxGSurfaceSetCurrentCvt2RgbaTargetToTexture(void);
void nxGSurfaceBackupCvt2RgbaTargetToTexture(GLuint texture_org);
#ifdef VR_FEATURE_OUTPUT_TEXTURE_USE
HSURFBOUNDTARGET nxGSurfaceConnectCvt2RgbaTargetToTexture(unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hvmem_target);
GLuint nxGSurfaceGetCvt2RgbaTextureName(HSURFBOUNDTARGET hTargetTex);
unsigned int nxGSurfaceGetCvt2RgbaTextureUnit(HSURFBOUNDTARGET hTargetTex);
void nxGSurfaceDisconnectCvt2RgbaTargetToTexture(HSURFBOUNDTARGET target);
#endif
void  nxGSurfaceRunCvt2RgbaToTexture(HSURFBOUNDTARGET hTarget, HSURFSOURCE hSource);

HSURFTARGET nxGSurfaceCreateCvt2RgbaTarget(unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, void* hData, int is_native_video_buf_handle, int iIsDefault);
void nxGSurfaceDestroyCvt2RgbaTarget(HSURFTARGET hTarget, int iIsDefault);
void nxGSurfaceRunCvt2Rgba(HSURFTARGET hTarget, HSURFSOURCE hSource);

#ifdef __cplusplus
}
#endif

#endif

