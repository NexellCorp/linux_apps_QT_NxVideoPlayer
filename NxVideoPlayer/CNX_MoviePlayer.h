/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CNX_MoviePlayer_H
#define CNX_MoviePlayer_H

#include <QTime>
#include <QDebug>
#include "CNX_Util.h"
#include <NX_MoviePlay.h>
#include "CNX_SubtitleParser.h"

// Drm CtrlId, PlaneId
//#define LCD_CTRLID      26
//#define LCD_PLANEID     27

//#define HDMI_CTRLID      34
//#define HDMI_PLANEID     35

// Width, Height Info
#define DSP_LCD_WIDTH      1024
#define DSP_LCD_HEIGHT     600

#define DSP_HDMI_WIDTH     1920
#define DSP_HDMI_HEIGHT    1080

typedef enum
{
    StoppedState	= 0,
    PlayingState	= 1,
    PausedState		= 2,
    ReadyState		= 3,
}NX_MediaStatus;

enum
{
    DSP_MODE_DEFAULT = 0,
    DSP_MODE_LCD,
    DSP_MODE_HDMI,
    DSP_MODE_TVOUT,
    DSP_MODE_LCD_HDMI,
    DSP_MODE_LCD_TVOUT,
};

#define MAX_DISPLAY_CHANNEL     8

#define SOFT_VOLUME

class CNX_MoviePlayer
{

public:
    CNX_MoviePlayer();
    ~CNX_MoviePlayer();

public:
    //
    //MediaPlayer commomn Initialize , close
    //mediaType is MP_TRACK_VIDEO or MP_TRACK_AUDIO
    int InitMediaPlayer(	void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
                            void *pCbPrivate,
                            const char *pUri,
                            int mediaType,
                            void (*pCbQtUpdateImg)(void *pImg)
                            );

    int CloseHandle();

    //
    //MediaPlayer common Control
    int SetVolume(int volume);
    int Play();
    int Seek(qint64 position);
    int Pause();
    int Stop();

    //
    //MediaPlayer common information
    void PrintMediaInfo( const char* pUri );
    qint64 GetMediaPosition();
    qint64 GetMediaDuration();
    NX_MediaStatus GetState();

    //
    //MediaPlayer video information
    int GetVideoWidth( int track );
    int GetVideoHeight( int track );
    int SetDspPosition( int track, int x, int y, int width, int height );
    int SetDisplayMode( int track, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect, int dspMode );

    void DrmVideoMute(int bOnOff);

    int MakeThumbnail(const char *pInFile, const char *pOutFile, int maxWidth, int maxHeight, int timeRatio);
    int GetVideoPlane( int crtcIdx, int layerIdx, int findRgb, MP_DRM_PLANE_INFO *pDrmPlaneInfo );

private:
    //
    //MediaPlayer InitMediaPlayer
    int OpenHandle( void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
                    void *cbPrivate );

    int SetUri(const char *pUri);
    int GetMediaInfo();
    int AddTrack(int mediaType);
    int SetRenderCallBack(void (*pCbQtUpdateImg)(void *pImg));

    int AddTrackForVideo();
    int AddTrackForAudio();
    int AddVideoConfig( int track, int planeId, int ctrlId, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect );
    int AddVideoTrack( int track );
    int AddAudioTrack( int track );
    int GetTrackIndex( int trackType, int track );

    //
    //vars
    pthread_mutex_t	m_hLock;
    MP_HANDLE		m_hPlayer;

    MP_MEDIA_INFO	m_MediaInfo;
    MP_DSP_CONFIG	*m_pDspConfig[MAX_DISPLAY_CHANNEL];
    MP_DSP_CONFIG	m_SubInfo;

    int				m_iDspMode;
    int				m_iSubDspWidth;
    int				m_iSubDspHeight;
    int				m_iMediaType;
    int             m_bVideoMute;
    int 			m_bIsCbQtUpdateImg;
    MP_DRM_PLANE_INFO m_idPrimaryDisplay;
    MP_DRM_PLANE_INFO m_idSecondDisplay;

public:
    int IsCbQtUpdateImg();

public:
    //
    // Subtitle
    CNX_SubtitleParser* m_pSubtitleParser;
    pthread_mutex_t m_SubtitleLock;
    pthread_t m_subtitleThread;
    int m_iSubtitleSeekTime;

    void	CloseSubtitle();
    int		OpenSubtitle(char * subtitlePath);
    int		GetSubtitleStartTime();
    void	SetSubtitleIndex(int idx);
    int		GetSubtitleIndex();
    int		GetSubtitleMaxIndex();
    void	IncreaseSubtitleIndex();
    void	SeekSubtitle(int milliseconds);
    char*	GetSubtitleText();
    bool	IsSubtitleAvailable();
    const char*	GetBestSubtitleEncode();
    const char* GetBestStringEncode(const char* str);

private:
    //
    // Subtitle
    static void* ThreadWrapForSubtitleSeek(void *Obj);
    void SeekSubtitleThread(void);
};

#endif // CNX_MoviePlayer_H
