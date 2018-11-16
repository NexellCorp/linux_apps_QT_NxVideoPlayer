#include "CNX_MoviePlayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "[NxVideoPlayer]"
#include <NX_Log.h>

#define AUDIO_DEFAULT_DEVICE "plughw:0,0"
#define AUDIO_HDMI_DEVICE    "plughw:0,3"

//------------------------------------------------------------------------------
CNX_MoviePlayer::CNX_MoviePlayer()
    : m_hPlayer( NULL )
    , m_iDspMode( DSP_MODE_DEFAULT )
    , m_iSubDspWidth( 0 )
    , m_iSubDspHeight( 0 )
    , m_iMediaType( 0 )
    , m_bVideoMute( 0 )
    , m_pSubtitleParser(NULL)
    , m_iSubtitleSeekTime( 0 )
{
    int crtcIdx  = -1;
    int layerIdx = -1;
    int findRgb  = -1;  //1:rgb, 0:video

    pthread_mutex_init( &m_hLock, NULL );
    pthread_mutex_init( &m_SubtitleLock, NULL );

    memset(&m_MediaInfo, 0x00, sizeof(MP_MEDIA_INFO));
    m_pSubtitleParser = new CNX_SubtitleParser();

    m_idPrimaryDisplay.iConnectorID = -1;
    m_idPrimaryDisplay.iCrtcId      = -1;
    m_idPrimaryDisplay.iPlaneId     = -1;

    m_idSecondDisplay.iConnectorID = -1;
    m_idSecondDisplay.iCrtcId      = -1;
    m_idSecondDisplay.iPlaneId     = -1;

    crtcIdx  = 0;
    layerIdx = 1;
    findRgb  = 0;
    if( 0 > GetVideoPlane(crtcIdx, layerIdx, findRgb, &m_idPrimaryDisplay) )
    {
        NXLOGE( "cannot found video format for %dth crtc\n", crtcIdx );
    }

    crtcIdx  = 1;
    layerIdx = 1;
    findRgb  = 0;
    if( 0 > GetVideoPlane( crtcIdx, layerIdx, findRgb, &m_idSecondDisplay) )
    {
        NXLOGE( "cannot found video format for %dth crtc\n", crtcIdx );
    }
}

CNX_MoviePlayer::~CNX_MoviePlayer()
{
    pthread_mutex_destroy( &m_hLock );
    pthread_mutex_destroy( &m_SubtitleLock );
    if(m_pSubtitleParser)
    {
        delete m_pSubtitleParser;
        m_pSubtitleParser = NULL;
    }
}

//================================================================================================================
//public methods	commomn Initialize , close
int CNX_MoviePlayer::InitMediaPlayer(	void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
                                        void *pCbPrivate,
                                        const char *pUri,
                                        int mediaType,
                                        void (*pCbQtUpdateImg)(void *pImg)
                                        )
{
    CNX_AutoLock lock( &m_hLock );

    if(0 > OpenHandle(pCbEventCallback, pCbPrivate) )           return -1;
    if(0 > SetUri(pUri) )                                       return -1;
    if(0 > GetMediaInfo() )                                     return -1;
    if(0 > AddTrack(mediaType) )                                return -1;
    if(0 > SetRenderCallBack(pCbQtUpdateImg) )                  return -1;

    if(pCbQtUpdateImg != NULL)
    {
        m_bIsCbQtUpdateImg = 1;
    }
    else
    {
        m_bIsCbQtUpdateImg = 0;
        DrmVideoMute(m_bVideoMute);
    }

    //PrintMediaInfo(pUri);
    return 0;
}

int CNX_MoviePlayer::CloseHandle()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( MP_TRACK_VIDEO == m_iMediaType )
    {
        for( int i = 0; i < MAX_DISPLAY_CHANNEL; i++ )
        {
            if( m_pDspConfig[i] )
            {
                free( m_pDspConfig[i] );
            }
            m_pDspConfig[i] = NULL;
        }
    }

    NX_MPClose( m_hPlayer );

    m_hPlayer = NULL;

    return 0;
}

//================================================================================================================
//public methods	common Control
int CNX_MoviePlayer::SetVolume(int volume)
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT iResult = NX_MPSetVolume( m_hPlayer, volume );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPSetVolume() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::Play()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    MP_RESULT iResult = NX_MPPlay( m_hPlayer );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPPlay() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }

    return 0;
}

int CNX_MoviePlayer::Seek(qint64 position)
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    MP_RESULT iResult = NX_MPSeek( m_hPlayer, position );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPSeek() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::Pause()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT iResult = NX_MPPause( m_hPlayer );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE("%s(): Error! NX_MPPause() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }

    return 0;
}

int CNX_MoviePlayer::Stop()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    MP_RESULT iResult = NX_MPStop( m_hPlayer );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPStop() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return 0;
}

//================================================================================================================
//public methods	common information
qint64 CNX_MoviePlayer::GetMediaPosition()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    int64_t iPosition;
    MP_RESULT iResult = NX_MPGetPosition( m_hPlayer, &iPosition );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPGetPosition() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return (qint64)iPosition;
}

qint64 CNX_MoviePlayer::GetMediaDuration()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    int64_t duration;
    MP_RESULT iResult = NX_MPGetDuration( m_hPlayer, &duration );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPGetDuration() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return (qint64)duration;
}

NX_MediaStatus CNX_MoviePlayer::GetState()
{
    CNX_AutoLock lock( &m_hLock );
    if( NULL == m_hPlayer )
    {
        return StoppedState;
    }
    return (NX_MediaStatus)NX_GetState(m_hPlayer);
}

void CNX_MoviePlayer::PrintMediaInfo( const char *pUri )
{

    NXLOGD("####################################################################################################\n");
    NXLOGD( "FileName : %s\n", pUri );
    NXLOGD( "Media Info : Program( %d EA ), Video( %d EA ), Audio( %d EA ), Subtitle( %d EA ), Data( %d EA )\n",
            m_MediaInfo.iProgramNum, m_MediaInfo.iVideoTrackNum, m_MediaInfo.iAudioTrackNum, m_MediaInfo.iSubTitleTrackNum, m_MediaInfo.iDataTrackNum );

    for( int32_t i = 0; i < m_MediaInfo.iProgramNum; i++ )
    {
        NXLOGD( "Program Info #%d : Video( %d EA ), Audio( %d EA ), Subtitle( %d EA ), Data( %d EA ), Duration( %lld ms )\n",
                i, m_MediaInfo.ProgramInfo[i].iVideoNum, m_MediaInfo.ProgramInfo[i].iAudioNum, m_MediaInfo.ProgramInfo[i].iSubTitleNum, m_MediaInfo.ProgramInfo[i].iDataNum, m_MediaInfo.ProgramInfo[i].iDuration);

        if( 0 < m_MediaInfo.ProgramInfo[i].iVideoNum )
        {
            int num = 0;
            for( int j = 0; j < m_MediaInfo.ProgramInfo[i].iVideoNum + m_MediaInfo.ProgramInfo[i].iAudioNum; j++ )
            {
                MP_TRACK_INFO *pTrackInfo = &m_MediaInfo.ProgramInfo[i].TrackInfo[j];
                if( MP_TRACK_VIDEO == pTrackInfo->iTrackType )
                {
                    NXLOGD( "-. Video Track #%d : Index( %d ), Type( %d ), Resolution( %d x %d ), Duration( %lld ms )\n",
                            num++, pTrackInfo->iTrackIndex, (int)pTrackInfo->iCodecId, pTrackInfo->iWidth, pTrackInfo->iHeight, pTrackInfo->iDuration );
                }
            }
        }

        if( 0 < m_MediaInfo.ProgramInfo[i].iAudioNum )
        {
            int num = 0;
            for( int j = 0; j < m_MediaInfo.ProgramInfo[i].iVideoNum + m_MediaInfo.ProgramInfo[i].iAudioNum; j++ )
            {
                MP_TRACK_INFO *pTrackInfo = &m_MediaInfo.ProgramInfo[i].TrackInfo[j];
                if( MP_TRACK_AUDIO == pTrackInfo->iTrackType )
                {
                    NXLOGD( "-. Audio Track #%d : Index( %d ), Type( %d ), Ch( %d ), Samplerate( %d Hz ), Bitrate( %d bps ), Duration( %lld ms )\n",
                            num++, pTrackInfo->iTrackIndex, (int)pTrackInfo->iCodecId, pTrackInfo->iChannels, pTrackInfo->iSampleRate, pTrackInfo->iBitrate, pTrackInfo->iDuration );
                }
            }
        }
    }
    NXLOGD( "####################################################################################################\n");
}

//================================================================================================================
//public methods	video information
int CNX_MoviePlayer::GetVideoWidth( int track )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_MediaInfo.iVideoTrackNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iVideoTrackNum );
        return -1;
    }

    int width = -1, trackOrder = 0;

    for( int i = 0; i < m_MediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_MediaInfo.ProgramInfo[i].iVideoNum + m_MediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( MP_TRACK_VIDEO == m_MediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    width = m_MediaInfo.ProgramInfo[i].TrackInfo[j].iWidth;
                    return width;
                }
                trackOrder++;
            }
        }
    }

    return width;
}

int CNX_MoviePlayer::GetVideoHeight( int track )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_MediaInfo.iVideoTrackNum )
    {
        NXLOGE("%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iVideoTrackNum );
        return -1;
    }

    int height = -1, trackOrder = 0;

    for( int i = 0; i < m_MediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_MediaInfo.ProgramInfo[i].iVideoNum + m_MediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( MP_TRACK_VIDEO == m_MediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    height = m_MediaInfo.ProgramInfo[i].TrackInfo[j].iHeight;
                    return height;
                }
                trackOrder++;
            }
        }
    }

    return height;
}

int CNX_MoviePlayer::SetDspPosition( int track, int x, int y, int width, int height )
{
    CNX_AutoLock lock( &m_hLock );

    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_MediaInfo.iVideoTrackNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iVideoTrackNum );
        return -1;
    }

    MP_DSP_RECT rect;
    rect.iX	 = x;
    rect.iY	 = y;
    rect.iWidth = width;
    rect.iHeight= height;

    MP_RESULT iResult = NX_MPSetDspPosition( m_hPlayer, track, &rect );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE("%s(): Error! NX_MPSetDspPosition() Failed! (ret = %d)", __FUNCTION__, iResult);
    }

    return 0;
}

int CNX_MoviePlayer::SetDisplayMode( int track, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect, int dspMode )
{
    CNX_AutoLock lock( &m_hLock );

    memset(&m_SubInfo, 0, sizeof(MP_DSP_CONFIG));
    m_SubInfo.ctrlId = 34;
    m_SubInfo.planeId = 35;
    m_SubInfo.srcRect = srcRect;
    m_SubInfo.dstRect = dstRect;

    m_iSubDspWidth = dstRect.iWidth;
    m_iSubDspHeight = dstRect.iHeight;
    m_iDspMode = dspMode;
    int iResult = 0;
    if( m_hPlayer )
    {
        iResult =	NX_MPSetDspMode(m_hPlayer, track, &m_SubInfo, dspMode );
    }
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE("%s(): Error! NX_MPSetDspPosition() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return iResult;
}

//================================================================================================================
//private methods	for InitMediaPlayer
int CNX_MoviePlayer::OpenHandle( void (*pCbEventCallback)( void *privateDesc, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ ),
                                 void *cbPrivate )
{
    MP_RESULT iResult = NX_MPOpen( &m_hPlayer, pCbEventCallback, cbPrivate );

    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s: Error! Handle is not initialized!\n", __FUNCTION__ );
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::SetUri(const char *pUri)
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    MP_RESULT iResult = NX_MPSetUri( m_hPlayer, pUri );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPSetUri() Failed! (ret = %d, uri = %s)\n", __FUNCTION__, iResult, pUri );
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::GetMediaInfo()
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    MP_RESULT iResult = NX_MPGetMediaInfo( m_hPlayer, &m_MediaInfo );
    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPGetMediaInfo() Failed! (ret = %d)\n", __FUNCTION__, iResult );
        return -1;
    }

    return 0;
}

int CNX_MoviePlayer::AddTrack(int mediaType)
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }
    m_iMediaType = mediaType;
    int iResult = -1;

    if(MP_TRACK_VIDEO == mediaType)	iResult = AddTrackForVideo();
    if(MP_TRACK_AUDIO == mediaType)	iResult = AddTrackForAudio();

    if(0 > iResult ) return -1;

    return 0;
}

int CNX_MoviePlayer::AddTrackForVideo()
{
    //video
    if( m_MediaInfo.iVideoTrackNum <= 0 )
    {
        NXLOGE("Fail, this contents do not have video track.");
        return -1;
    }
    else
    {
        MP_DSP_RECT srcRect;
        MP_DSP_RECT dstRect;
        memset(&srcRect, 0, sizeof(MP_DSP_RECT));
        memset(&dstRect, 0, sizeof(MP_DSP_RECT));

        if( m_iDspMode == DSP_MODE_LCD_HDMI)
        {
            dstRect.iWidth   = m_iSubDspWidth;
            dstRect.iHeight  = m_iSubDspHeight;

            if( 0 > AddVideoConfig( 0, m_idSecondDisplay.iPlaneId, m_idSecondDisplay.iCrtcId, srcRect, dstRect ) )
            {
                NXLOGE( "%s: Error! AddVideoConfig()\n", __FUNCTION__);
                return -1;
            }

            // <<-- FIXED VIDEO TRACK -->>
            if( 0 > AddVideoTrack(0) )
            {
                NXLOGE( "%s: Error! AddVideoTrack()\n", __FUNCTION__);
                return -1;
            }

            if( 0 >	NX_MPSetDspMode(m_hPlayer, 0, &m_SubInfo, m_iDspMode ) )
            {
                NXLOGE( "%s: Error! NX_MPSetDspMode()\n", __FUNCTION__);
                return -1;
            }
        }
        else
        {
            dstRect.iWidth   = DSP_LCD_WIDTH;
            dstRect.iHeight  = DSP_LCD_HEIGHT;

            if( 0 > AddVideoConfig( 0, m_idPrimaryDisplay.iPlaneId, m_idPrimaryDisplay.iCrtcId, srcRect, dstRect ) )
            {
                NXLOGE( "%s: Error! AddVideoConfig()\n", __FUNCTION__);
                return -1;
            }

            // <<-- FIXED AUDIO TRACK -->>
            if( 0 > AddVideoTrack(0) )
            {
                NXLOGE( "%s: Error! AddVideoTrack()\n", __FUNCTION__);
                return -1;
            }
        }
    }

    //audio
    if( 0 > AddTrackForAudio() )
    {
        return -1;
    }

    return 0;
}

int CNX_MoviePlayer::SetRenderCallBack(void (*pCbQtUpdateImg)(void *pImg))
{
    if( NULL == pCbQtUpdateImg)
    {
        return 0;
    }

    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    MP_RESULT iResult = NX_MPSetRenderCallBack( m_hPlayer, 0, pCbQtUpdateImg );

    if(MP_ERR_NONE != iResult)
    {
        NXLOGE( "%s(): Error! NX_MPSetRenderCallBack() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::AddTrackForAudio()
{
    //audio
    if( m_MediaInfo.iAudioTrackNum <= 0 )
    {
        NXLOGE("Fail, this contents do not have audio track.");
        return -1;
    }
    else
    {
        // <<-- FIXED AUDIO TRACK -->>
        if( 0 > AddAudioTrack(0) )
            return -1;
    }
    return 0;
}

int CNX_MoviePlayer::AddVideoConfig( int track, int planeId, int ctrlId, MP_DSP_RECT srcRect, MP_DSP_RECT dstRect )
{
    if( track >= m_MediaInfo.iVideoTrackNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iVideoTrackNum );
        return -1;
    }

    for( int i = 0; i < MAX_DISPLAY_CHANNEL; i++ )
    {
        m_pDspConfig[i] = NULL;
    }

    if( NULL != m_pDspConfig[track] )
    {
        NXLOGE( "%s(): Error! VideoConfig Slot is not empty.", __FUNCTION__ );
        return -1;
    }

    m_pDspConfig[track] = (MP_DSP_CONFIG*)malloc( sizeof(MP_DSP_CONFIG) );
    m_pDspConfig[track]->planeId	= planeId;
    m_pDspConfig[track]->ctrlId		= ctrlId;

    m_pDspConfig[track]->srcRect				= srcRect;
    m_pDspConfig[track]->dstRect				= dstRect;

    return 0;
}

int CNX_MoviePlayer::AddVideoTrack( int track )
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_MediaInfo.iVideoTrackNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / videoTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iVideoTrackNum );
        return -1;
    }

    int index = GetTrackIndex( MP_TRACK_VIDEO, track );
    if( 0 > index )
    {
        NXLOGE( "%s(): Error! Get Video Index. ( track = %d )", __FUNCTION__, track );
        return -1;
    }

    if( NULL == m_pDspConfig[track] )
    {
        NXLOGE( "%s(): Error! Invalid VideoConfig.", __FUNCTION__ );
        return -1;
    }

    MP_RESULT iResult = NX_MPAddVideoTrack( m_hPlayer, index, m_pDspConfig[track] );

    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPAddVideoTrack() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }

    return 0;
}

int CNX_MoviePlayer::AddAudioTrack( int track )
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return -1;
    }

    if( track >= m_MediaInfo.iAudioTrackNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / audioTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.iAudioTrackNum );
        return -1;
    }

    if( track >= m_MediaInfo.ProgramInfo[0].iAudioNum )
    {
        NXLOGE( "%s(): Error! Track Number. (track = %d / audioTrack = %d)\n", __FUNCTION__, track, m_MediaInfo.ProgramInfo[0].iAudioNum );
        return -1;
    }

    int index = GetTrackIndex( MP_TRACK_AUDIO, track );
    if( 0 > index )
    {
        NXLOGE(  "%s(): Error! Get Audio Index. ( track = %d )", __FUNCTION__, track );
        return -1;
    }

    MP_RESULT iResult = NX_MPAddAudioTrack( m_hPlayer, index, NULL, AUDIO_DEFAULT_DEVICE );

    if( MP_ERR_NONE != iResult )
    {
        NXLOGE( "%s(): Error! NX_MPAddAudioTrack() Failed! (ret = %d)", __FUNCTION__, iResult);
        return -1;
    }
    return 0;
}

int CNX_MoviePlayer::GetTrackIndex( int trackType, int track )
{
    int index = -1, trackOrder = 0;

    for( int i = 0; i < m_MediaInfo.iProgramNum; i++ )
    {
        for( int j = 0; j < m_MediaInfo.ProgramInfo[i].iVideoNum + m_MediaInfo.ProgramInfo[i].iAudioNum; j++ )
        {
            if( trackType == m_MediaInfo.ProgramInfo[i].TrackInfo[j].iTrackType )
            {
                if( track == trackOrder )
                {
                    index = m_MediaInfo.ProgramInfo[i].TrackInfo[j].iTrackIndex;
                    // qDebug( "[%s] Require Track( %d ), Stream Index( %d )", (trackType == MP_TRACK_AUDIO) ? "AUDIO" : "VIDEO", track, index );
                    return index;
                }
                trackOrder++;
            }
        }
    }

    return index;
}

//================================================================================================================
// subtitle routine
void CNX_MoviePlayer::CloseSubtitle()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser)
    {
        if(m_pSubtitleParser->NX_SPIsParsed())
        {
            m_pSubtitleParser->NX_SPClose();
        }
    }
}

int CNX_MoviePlayer::OpenSubtitle(char * subtitlePath)
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser)
    {
        return m_pSubtitleParser->NX_SPOpen(subtitlePath);
    }else
    {
        NXLOGW("in OpenSubtitle no parser instance\n");
        return 0;
    }
}

int CNX_MoviePlayer::GetSubtitleStartTime()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        return m_pSubtitleParser->NX_SPGetStartTime();
    }else
    {
        return 0;
    }
}

void CNX_MoviePlayer::SetSubtitleIndex(int idx)
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        m_pSubtitleParser->NX_SPSetIndex(idx);
    }
}

int CNX_MoviePlayer::GetSubtitleIndex()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        return m_pSubtitleParser->NX_SPGetIndex();
    }else
    {
        return 0;
    }
}

int CNX_MoviePlayer::GetSubtitleMaxIndex()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        return m_pSubtitleParser->NX_SPGetMaxIndex();
    }else
    {
        return 0;
    }
}

void CNX_MoviePlayer::IncreaseSubtitleIndex()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        m_pSubtitleParser->NX_SPIncreaseIndex();
    }
}

char* CNX_MoviePlayer::GetSubtitleText()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        return m_pSubtitleParser->NX_SPGetSubtitle();
    }else
    {
        return NULL;
    }
}

bool CNX_MoviePlayer::IsSubtitleAvailable()
{
    return m_pSubtitleParser->NX_SPIsParsed();
}

const char *CNX_MoviePlayer::GetBestSubtitleEncode()
{
    CNX_AutoLock lock( &m_SubtitleLock );
    if(m_pSubtitleParser->NX_SPIsParsed())
    {
        return m_pSubtitleParser->NX_SPGetBestTextEncode();
    }else
    {
        return NULL;
    }
}

const char *CNX_MoviePlayer::GetBestStringEncode(const char *str)
{
    if(!m_pSubtitleParser)
    {
        NXLOGW("GetBestStringEncode no parser instance\n");
        return "EUC-KR";
    }else
    {
        return m_pSubtitleParser->NX_SPFindStringEncode(str);
    }
}

void CNX_MoviePlayer::SeekSubtitle(int milliseconds)
{
    if (0 > pthread_create(&m_subtitleThread, NULL, ThreadWrapForSubtitleSeek, this) )
    {
        NXLOGE("SeekSubtitle creating Thread err\n");
        m_pSubtitleParser->NX_SPSetIndex(0);
        return;
    }

    m_iSubtitleSeekTime = milliseconds;
    NXLOGD("seek input  : %d\n",milliseconds);

    pthread_join(m_subtitleThread, NULL);
}

void* CNX_MoviePlayer::ThreadWrapForSubtitleSeek(void *Obj)
{
    if( NULL != Obj )
    {
        NXLOGD("ThreadWrapForSubtitleSeek ok\n");
        ( (CNX_MoviePlayer*)Obj )->SeekSubtitleThread();
    }else
    {
        NXLOGE("ThreadWrapForSubtitleSeek err\n");
        return (void*)0xDEADDEAD;
    }
    return (void*)1;
}

void CNX_MoviePlayer::SeekSubtitleThread(void)
{
    CNX_AutoLock lock( &m_SubtitleLock );
    m_pSubtitleParser->NX_SPSetIndex(m_pSubtitleParser->NX_SPSeekSubtitleIndex(m_iSubtitleSeekTime));
}

//================================================================================================================
void CNX_MoviePlayer::DrmVideoMute(int bOnOff)
{
    if( NULL == m_hPlayer )
    {
        NXLOGE( "%s: Error! Handle is not initialized!", __FUNCTION__ );
        return;
    }

    m_bVideoMute = bOnOff;
    NX_MPVideoMute(m_hPlayer, m_bVideoMute, m_pDspConfig[0]);
}

int CNX_MoviePlayer::IsCbQtUpdateImg()
{
    return m_bIsCbQtUpdateImg;
}

//================================================================================================================
int CNX_MoviePlayer::MakeThumbnail(const char *pInFile, const char *pOutFile, int maxWidth, int maxHeight, int timeRatio)
{
    int ret = 0;
    ret = NX_MPMakeThumbnail( pInFile, pOutFile, maxWidth, maxHeight, timeRatio );

    return ret;
}

//================================================================================================================
int CNX_MoviePlayer::GetVideoPlane( int crtcIdx, int layerIdx, int findRgb, MP_DRM_PLANE_INFO *pDrmPlaneInfo )
{
    int ret = 0;
    ret = NX_MPGetPlaneForDisplay( crtcIdx, layerIdx, findRgb, pDrmPlaneInfo );

    return ret;
}
