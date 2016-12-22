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

#ifndef NX_CMEDIAPLAYER_H
#define NX_CMEDIAPLAYER_H

#include <QTime>
#include <QDebug>

#include <NX_MoviePlay.h>

// Drm CtrlId, PlaneId
#define LCD_CTRLID      26
#define LCD_PLANEID     27

#define HDMI_CTRLID      34
#define HDMI_PLANEID     35

// Width, Height Info
#define DSP_LCD_WIDTH      1024
#define DSP_LCD_HEIGHT     600

#define DSP_HDMI_WIDTH     1920
#define DSP_HDMI_HEIGHT    1080


typedef enum
{
	StoppedState    = 0,
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

class NX_CMediaPlayer
{

public:
	NX_CMediaPlayer();
	~NX_CMediaPlayer();

public:
	int setMedia(QString Uri, void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
				void (*cbQtUpdateImg)(void *pImg));     //Set Uri
	int pause();                        //Pause
	int play();                         //Play
	int setPosition(qint64 position);   //Seek Position
	int setVolume(int volume);          //Set Volume
	int stop();                         //Stop
	int close();                        //Close
    //Get Infomation
	NX_MediaStatus	state();            //Get state (stop,play,pause)
	qint64  duration();                 //Get Duration (ms)
	qint64  position();                 //Get Position (ms)
	int volume();                       //Get Volume: 0 ~ 100, 0:mute
    //
	int getVideoWidth( int track );
	int getVideoHeight( int track );
	int getVersion();
	int setDspPosition( int iTrack, int x, int y, int width, int height );
	int setDisplayMode( int track,  MP_DSP_RECT srcRect, MP_DSP_RECT dstRect, int dspMode );
	void PrintMediaInfo( void );

private:
	int open(const char *pUri, void (*m_pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ) );
	int getTrackIndex( int trackType, int track );
	int getVideoTrackNum();
	int getAudioTrackNum();
	int addAudioTrack( int track );
	int addVideoTrack( int track );
	int addVideoConfig( int track, int planeId, int ctrlId, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect );

public:
	MP_HANDLE       m_hPlayer;
	MP_MEDIA_INFO   m_mediaInfo;
	char            m_uriBuf[256];
	MP_DSP_CONFIG   *m_pDspConfig[MAX_DISPLAY_CHANNEL];
	NX_MediaStatus  m_isStatus;
	int             m_dspMode;
	int             m_subDspWidth;
	int             m_subDspHeight;
	pthread_mutex_t m_hLock;
	void (*m_pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ );
	MP_DSP_CONFIG   m_subInfo;
};

#endif // NX_CMEDIAPLAYER_H
