#include "NX_CMediaPlayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define AUDIO_DEFAULT_DEVICE "plughw:0,0"
#define AUDIO_HDMI_DEVICE    "plughw:0,3"

//------------------------------------------------------------------------------
//
//  Fucntion Lock
//
class CNX_AutoLock {
public:
	CNX_AutoLock( pthread_mutex_t *pLock )
		: m_pLock(pLock)
	{
		pthread_mutex_lock( m_pLock );
	}
	~CNX_AutoLock()
	{
		pthread_mutex_unlock( m_pLock );
	}

protected:
	pthread_mutex_t *m_pLock;

private:
	CNX_AutoLock (const CNX_AutoLock &Ref);
	CNX_AutoLock &operator=(CNX_AutoLock &Ref);
};


//------------------------------------------------------------------------------
NX_CMediaPlayer::NX_CMediaPlayer()
	: m_hPlayer( NULL )
	, m_isStatus( StoppedState )
	, m_dspMode( DSP_MODE_DEFAULT )
	, m_subDspWidth( 0 )
	, m_subDspHeight( 0 )
{
	pthread_mutex_init( &m_hLock, NULL );
	memset(&m_mediaInfo, 0x00, sizeof(MP_MEDIA_INFO));
}

NX_CMediaPlayer::~NX_CMediaPlayer()
{
	pthread_mutex_destroy( &m_hLock );
}

void NX_CMediaPlayer::PrintMediaInfo( void )
{

	qDebug("####################################################################################################\n");
	qDebug( "FileName : %s\n", m_uriBuf );
	qDebug( "Media Info : Program( %d EA ), Video( %d EA ), Audio( %d EA ), Subtitle( %d EA ), Data( %d EA )\n",
			m_mediaInfo.iProgramNum, m_mediaInfo.iVideoTrackNum, m_mediaInfo.iAudioTrackNum, m_mediaInfo.iSubTitleTrackNum, m_mediaInfo.iDataTrackNum );

	for( int32_t i = 0; i < m_mediaInfo.iProgramNum; i++ )
	{
		qDebug( "Program Info #%d : Video( %d EA ), Audio( %d EA ), Subtitle( %d EA ), Data( %d EA ), Duration( %lld ms )\n",
				i, m_mediaInfo.ProgramInfo[i].iVideoNum, m_mediaInfo.ProgramInfo[i].iAudioNum, m_mediaInfo.ProgramInfo[i].iSubTitleNum, m_mediaInfo.ProgramInfo[i].iDataNum, m_mediaInfo.ProgramInfo[i].iDuration);

		if( 0 < m_mediaInfo.ProgramInfo[i].iVideoNum )
		{
			int num = 0;
			for( int j = 0; j < m_mediaInfo.ProgramInfo[i].iVideoNum + m_mediaInfo.ProgramInfo[i].iAudioNum; j++ )
			{
				MP_TRACK_INFO *pTrackInfo = &m_mediaInfo.ProgramInfo[i].TrackInfo[j];
				if( MP_TRACK_VIDEO == pTrackInfo->iTrackType )
				{
					qDebug( "-. Video Track #%d : Index( %d ), Type( %d ), Resolution( %d x %d ), Duration( %lld ms )\n",
							num++, pTrackInfo->iTrackIndex, (int)pTrackInfo->iCodecId, pTrackInfo->iWidth, pTrackInfo->iHeight, pTrackInfo->iDuration );
				}
			}
		}

		if( 0 < m_mediaInfo.ProgramInfo[i].iAudioNum )
		{
			int num = 0;
			for( int j = 0; j < m_mediaInfo.ProgramInfo[i].iVideoNum + m_mediaInfo.ProgramInfo[i].iAudioNum; j++ )
			{
				MP_TRACK_INFO *pTrackInfo = &m_mediaInfo.ProgramInfo[i].TrackInfo[j];
				if( MP_TRACK_AUDIO == pTrackInfo->iTrackType )
				{
					qDebug( "-. Audio Track #%d : Index( %d ), Type( %d ), Ch( %d ), Samplerate( %d Hz ), Bitrate( %d bps ), Duration( %lld ms )\n",
							num++, pTrackInfo->iTrackIndex, (int)pTrackInfo->iCodecId, pTrackInfo->iChannels, pTrackInfo->iSampleRate, pTrackInfo->iBitrate, pTrackInfo->iDuration );
				}
			}
		}
	}
	qDebug( "####################################################################################################\n");
}


int NX_CMediaPlayer::setMedia(QString Uri, void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
                              void (*pCbQtUpdateImg)(void *pImg) )

{
	CNX_AutoLock lock( &m_hLock );

	int ret = 0;

	ret = open(Uri.toStdString().c_str(), pCbEventCallback );
	if(ret != 0)
	{
		qDebug( "%s: Error! open(), uri = %s\n", __FUNCTION__,  Uri.toUtf8().constData() );
		return -1;
	}

	if( getVideoTrackNum() > 0 )
	{
		MP_DSP_RECT srcRect;
		MP_DSP_RECT dstRect;
		memset(&srcRect, 0, sizeof(MP_DSP_RECT));
		memset(&dstRect, 0, sizeof(MP_DSP_RECT));

		if( m_dspMode == DSP_MODE_LCD_HDMI)
		{
			dstRect.iWidth   = m_subDspWidth;
			dstRect.iHeight  = m_subDspHeight;

            if( 0 > addVideoConfig( 0, HDMI_PLANEID, HDMI_CTRLID, srcRect, dstRect ) )
            {
                qDebug( "%s: Error! addVideoConfig()\n", __FUNCTION__);
                return -1;
            }

            // <<-- FIXED VIDEO TRACK -->>
            if( 0 > addVideoTrack(0) )
            {
                qDebug( "%s: Error! addVideoTrack()\n", __FUNCTION__);
                return -1;
            }

            NX_MPSetRenderCallBack( m_hPlayer, 0, pCbQtUpdateImg );

            if( 0 >	NX_MPSetDspMode(m_hPlayer, 0, &m_subInfo, m_dspMode ) )
            {
                qDebug( "%s: Error! NX_MPSetDspMode()\n", __FUNCTION__);
                return -1;
            }
        }
        else
        {
            dstRect.iWidth   = DSP_LCD_WIDTH;
            dstRect.iHeight  = DSP_LCD_HEIGHT;

            if( 0 > addVideoConfig( 0, LCD_PLANEID, LCD_CTRLID, srcRect, dstRect ) )
            {
                qDebug( "%s: Error! addVideoConfig()\n", __FUNCTION__);
                return -1;
            }

            // <<-- FIXED AUDIO TRACK -->>
            if( 0 > addVideoTrack(0) )
            {
                qDebug( "%s: Error! addVideoTrack()\n", __FUNCTION__);
                return -1;
            }
            NX_MPSetRenderCallBack( m_hPlayer, 0, pCbQtUpdateImg );
        }

    }
    else
    {
        qDebug("Fail, this contents do not have video track.");
        return -1;
    }

    if( getAudioTrackNum() > 0 )
    {
        // <<-- FIXED AUDIO TRACK -->>
        if( 0 > addAudioTrack(0) )
            return -1;
    }
    else
    {
        qDebug("Fail, this contents do not have audio track.");
        return -1;
    }

    m_isStatus = ReadyState;

    return 0;
}

int NX_CMediaPlayer::pause()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPPause( m_hPlayer );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug("%s(): Error! NX_MPPause() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    m_isStatus = PausedState;

    return 0;
}

int NX_CMediaPlayer::play()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPPlay( m_hPlayer );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPPlay() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    m_isStatus = PlayingState;

    return 0;
}

int NX_CMediaPlayer::setPosition(qint64 position)
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPSeek( m_hPlayer, position );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPSeek() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return mpResult;
}

int NX_CMediaPlayer::setVolume(int volume)
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPSetVolume( m_hPlayer, volume );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPSetVolume() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return 0;
}

int NX_CMediaPlayer::stop()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPStop( m_hPlayer );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPStop() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    m_isStatus = StoppedState;

    return 0;
}

int NX_CMediaPlayer::close()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT mpResult = NX_MPClearTrack( m_hPlayer );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPClearTrack() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    for( int i = 0; i < MAX_DISPLAY_CHANNEL; i++ )
    {
        if( m_pDspConfig[i] )
        {
            free( m_pDspConfig[i] );
        }
        m_pDspConfig[i] = NULL;
    }

    NX_MPClose( m_hPlayer );
    m_hPlayer = NULL;

    m_isStatus = StoppedState;

    return 0;
}

NX_MediaStatus	NX_CMediaPlayer::state()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
//        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return StoppedState;
    }

    return m_isStatus;
}

qint64  NX_CMediaPlayer::duration()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    int64_t duration;
    MP_RESULT mpResult = NX_MPGetDuration( m_hPlayer, &duration );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPGetDuration() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return (qint64)duration;
}

qint64  NX_CMediaPlayer::position()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    int64_t position;
    MP_RESULT mpResult = NX_MPGetPosition( m_hPlayer, &position );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPGetPosition() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return (qint64)position;
}

int NX_CMediaPlayer::volume()
{
    CNX_AutoLock lock( &m_hLock );

    return 0;
}

int NX_CMediaPlayer::getVideoWidth( int track )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iVideoTrackNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iVideoTrackNum );
        return -1;
    }

    int width = -1, trackOrder = 0;

    for( int i = 0; i < m_mediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_mediaInfo.ProgramInfo[i].iVideoNum + m_mediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( MP_TRACK_VIDEO == m_mediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    width = m_mediaInfo.ProgramInfo[i].TrackInfo[j].iWidth;
                    return width;
                }
                trackOrder++;
            }
        }
    }

    return width;
}

int NX_CMediaPlayer::getVideoHeight( int track )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iVideoTrackNum )
    {
        qDebug("%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iVideoTrackNum );
        return -1;
    }

    int height = -1, trackOrder = 0;

    for( int i = 0; i < m_mediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_mediaInfo.ProgramInfo[i].iVideoNum + m_mediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( MP_TRACK_VIDEO == m_mediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    height = m_mediaInfo.ProgramInfo[i].TrackInfo[j].iHeight;
                    return height;
                }
                trackOrder++;
            }
        }
    }

    return height;
}

int NX_CMediaPlayer::getVersion()
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    return NX_MPGetVersion();
}

int NX_CMediaPlayer::setDspPosition( int track, int x, int y, int width, int height )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iVideoTrackNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iVideoTrackNum );
        return -1;
    }

    MP_DSP_RECT rect;
    rect.iX     = x;
    rect.iY     = y;
    rect.iWidth = width;
    rect.iHeight= height;

    MP_RESULT mpResult = NX_MPSetDspPosition( m_hPlayer, track, &rect );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug("%s(): Error! NX_MPSetDspPosition() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return 0;
}

int NX_CMediaPlayer::setDisplayMode( int track, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect, int dspMode )
{
    int ret = 0;
    CNX_AutoLock lock( &m_hLock );

    memset(&m_subInfo, 0, sizeof(MP_DSP_CONFIG));
    m_subInfo.ctrlId = 34;
    m_subInfo.planeId = 35;
    m_subInfo.srcRect = srcRect;
    m_subInfo.dstRect = dstRect;

    m_subDspWidth = dstRect.iWidth;
    m_subDspHeight = dstRect.iHeight;
    m_dspMode = dspMode;

    if( m_hPlayer )
    {
        ret =	NX_MPSetDspMode(m_hPlayer, track, &m_subInfo, dspMode );
    }

    return ret;
}

//------------------------------------------------------------------------------
int NX_CMediaPlayer::open(const char *pUri, void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ) )
{
    MP_RESULT mpResult = MP_ERR_NONE;
    mpResult = NX_MPOpen( &m_hPlayer, pCbEventCallback, NULL );

    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s: Error! Handle is not initialized!\n", __FUNCTION__ );
        return -1;
    }

    strcpy( m_uriBuf, pUri );

    qDebug("%s(): UriName:: %s\n", __FUNCTION__, m_uriBuf );

    mpResult = NX_MPSetUri( m_hPlayer, m_uriBuf );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPSetUri() Failed! (ret = %d, uri = %s)\n", __FUNCTION__, mpResult, m_uriBuf );
    }
    else
    {
        mpResult = NX_MPGetMediaInfo( m_hPlayer, &m_mediaInfo );
        if( MP_ERR_NONE != mpResult )
        {
            qDebug( "%s(): Error! NX_MPGetMediaInfo() Failed! (ret = %d)\n", __FUNCTION__, mpResult );
        }
    }

    for( int i = 0; i < MAX_DISPLAY_CHANNEL; i++ )
    {
        m_pDspConfig[i] = NULL;
    }

    PrintMediaInfo();

    return mpResult;
}

//------------------------------------------------------------------------------
int NX_CMediaPlayer::getTrackIndex( int trackType, int track )
{
    int index = -1, trackOrder = 0;

    for( int i = 0; i < m_mediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_mediaInfo.ProgramInfo[i].iVideoNum + m_mediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( trackType == m_mediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    index = m_mediaInfo.ProgramInfo[i].TrackInfo[j].iTrackIndex;
                    // qDebug( "[%s] Require Track( %d ), Stream Index( %d )", (trackType == MP_TRACK_AUDIO) ? "AUDIO" : "VIDEO", track, index );
                    return index;
                }
                trackOrder++;
            }
        }
    }

    return index;
}

//------------------------------------------------------------------------------
int NX_CMediaPlayer::getVideoTrackNum( )
{
    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    return m_mediaInfo.iVideoTrackNum;
}

//------------------------------------------------------------------------------
int NX_CMediaPlayer::getAudioTrackNum()
{
    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    return m_mediaInfo.iAudioTrackNum;
}

int NX_CMediaPlayer::addAudioTrack( int track )
{
    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iAudioTrackNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / audioTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iAudioTrackNum );
        return -1;
    }

    if( track >= m_mediaInfo.ProgramInfo[0].iAudioNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / audioTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.ProgramInfo[0].iAudioNum );
        return -1;
    }

    int index = getTrackIndex( MP_TRACK_AUDIO, track );
    if( 0 > index )
    {
        qDebug(  "%s(): Error! Get Audio Index. ( track = %d )", __FUNCTION__, track );
        return -1;
    }

    MP_RESULT mpResult = NX_MPAddTrack1( m_hPlayer, index, NULL, AUDIO_DEFAULT_DEVICE );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPAddTrack() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }
    return mpResult;
}

int NX_CMediaPlayer::addVideoTrack( int track )
{
    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iVideoTrackNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iVideoTrackNum );
        return -1;
    }

    int index = getTrackIndex( MP_TRACK_VIDEO, track );
    if( 0 > index )
    {
        qDebug( "%s(): Error! Get Video Index. ( track = %d )", __FUNCTION__, track );
        return -1;
    }

    if( NULL == m_pDspConfig[track] )
    {
        qDebug( "%s(): Error! Invalid VideoConfig.", __FUNCTION__ );
        return -1;
    }
    MP_RESULT   mpResult = NX_MPAddTrack( m_hPlayer, index, m_pDspConfig[track] );
    if( MP_ERR_NONE != mpResult )
    {
        qDebug( "%s(): Error! NX_MPAddTrack() Failed! (ret = %d)", __FUNCTION__, mpResult);
    }

    return mpResult;
}

int NX_CMediaPlayer::addVideoConfig( int track, int planeId, int ctrlId, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect )
{
    if( NULL == m_hPlayer )
    {
        qDebug( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_mediaInfo.iVideoTrackNum )
    {
        qDebug( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_mediaInfo.iVideoTrackNum );
        return -1;
    }

    if( NULL != m_pDspConfig[track] )
    {
        qDebug( "%s(): Error! VideoConfig Slot is not empty.", __FUNCTION__ );
        return -1;
    }

    m_pDspConfig[track] = (MP_DSP_CONFIG*)malloc( sizeof(MP_DSP_CONFIG) );
    m_pDspConfig[track]->planeId        = planeId;
    m_pDspConfig[track]->ctrlId         = ctrlId;

    m_pDspConfig[track]->srcRect        = srcRect;
    m_pDspConfig[track]->dstRect        = dstRect;

    return 0;
}
