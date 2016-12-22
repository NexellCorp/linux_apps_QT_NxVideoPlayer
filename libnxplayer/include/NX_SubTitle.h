//------------------------------------------------------------------------------
//
//	Copyright (C) 2015 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		:
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NX_SUBTITLE_H__
#define __NX_SUBTITLE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t NX_SubTitleOpen(void **pphandle, char *pFileName);
int32_t NX_SubTitleDecode(void *phandle, char *pOutBuf, int32_t *pOutBufSize);
void NX_SubTitleClose(void *phandle);

#ifdef __cplusplus
}
#endif

#endif	// __NX_SUBTITLE_H__
