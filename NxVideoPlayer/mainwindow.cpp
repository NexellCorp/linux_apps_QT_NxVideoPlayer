#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextCodec>

static const char *NX_VIDEO_EXTENSION[] = {
	".mp4",  ".avi",  ".mkv",
	".divx", ".rmvb", ".flv",
};

//Display Mode
#define DSP_FULL   0
#define DSP_HALF   1

//Display Info
#define DSP_FULL_WIDTH  1024
#define DSP_FULL_HEIGHT 600
#define DSP_HALF_WIDTH  DSP_FULL_WIDTH/2
#define DSP_HALF_HEIGHT 600

//Button Width,Height
#define BUTTON_Y            520
#define BUTTON_FULL_X       20
#define BUTTON_FULL_WIDTH   90
#define BUTTON_FULL_HEIGHT  60
#define BUTTON_HALF_X       10
#define BUTTON_HALF_WIDTH   40
#define BUTTON_HALF_HEIGHT  60


////////////////////////////////////////////////////////////////////////////////
//
//	Event Callback
//
static    CallBackSignal mediaStateCb;

//CallBack Eos, Error
static void cbEventCallback( void */*privateDesc*/, unsigned int EventType, unsigned int /*EventData*/, unsigned int /*param*/ )
{
	mediaStateCb.statusChanged(EventType);
}

////////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, m_duration    (0)
	, m_savePosition(0)
	, m_volValue    (30)
	, m_fileIndex   (0)
	, m_pNxPlayer   (NULL)
	, m_DspMode     (DSP_MODE_DEFAULT)
	, m_bFindVideoFile (false)
	, m_bSeekReady  (false)
	, m_bVoumeCtrlReady (false)
	, m_bButtonHide (false)
	, m_curFileListIdx (0)
	, m_scrWidth(DSP_FULL_WIDTH)
	, m_scrHeight(DSP_FULL_HEIGHT)
	, m_pSubtitleParser (NULL)
	, m_bSubThreadFlag (NULL)
	, m_pSubTitleTimer (NULL)
	, ui(new Ui::MainWindow)
{
	int ret = 0;

	ui->setupUi(this);

	//Find file list
	m_fileList.MakeFileList("/run/media",(const char **)&NX_VIDEO_EXTENSION, sizeof(NX_VIDEO_EXTENSION) / sizeof(NX_VIDEO_EXTENSION[0]) );

	qDebug("<<< Total file list = %d\n", m_fileList.GetSize());

    //	Connect Solt Functions
	connect(&mediaStateCb, SIGNAL(mediaStatusChanged(int)), SLOT(statusChanged(int)));

	// Initialize Player
	if(  m_fileList.GetSize() > 0)
	{
        m_pNxPlayer = new CNX_MoviePlayer();
		m_DspMode = DSP_MODE_DEFAULT;
#if 0
		m_DspMode = DSP_MODE_LCD_HDMI;
		if(m_DspMode == DSP_MODE_LCD_HDMI)
		{
			MP_DSP_RECT srcRect;
			MP_DSP_RECT dstRect;
			memset(&srcRect, 0, sizeof(MP_DSP_RECT));
			memset(&dstRect, 0, sizeof(MP_DSP_RECT));
			dstRect.iWidth = DSP_HDMI_WIDTH;
			dstRect.iHeight = DSP_HDMI_HEIGHT;
			m_pNxPlayer->setDisplayMode( 0, srcRect, dstRect, m_DspMode );
		}
#endif

        while(true)
		{
            QString Uri = m_fileList.GetList(m_fileIndex);
            ret = m_pNxPlayer->InitMediaPlayer( cbEventCallback,
                                                NULL,
                                                Uri.toStdString().c_str(),
                                                MP_TRACK_VIDEO,
                                                NULL
                                               );

			if(ret < 0)
			{
                m_pNxPlayer->Stop();
                m_pNxPlayer->CloseHandle();

				if( (m_fileList.GetSize()-1 == m_fileIndex) || ( 1 == m_fileList.GetSize() ) )
				{
					m_fileIndex = 0;
					m_curFileListIdx = m_fileIndex;
					m_bFindVideoFile = false;
					delete m_pNxPlayer;
					m_pNxPlayer = NULL;
					break;
				}
				else
				{
					m_fileIndex++;
					m_curFileListIdx = m_fileIndex;
				}
			}
			else
			{
				m_bFindVideoFile = true;
                if( 0 == OpenSubTitle() )
                {
                    PlaySubTitle();
                }
				break;
			}
		}

		if(m_pNxPlayer)
		{
			m_curFileListIdx = m_fileIndex;
		}
	}

	//Init Volume
	ui->volumeProgressBar->setValue(m_volValue);
	ui->volumelabel->setText(QString("%1").arg(m_volValue));
	if(m_pNxPlayer)
	{
        m_pNxPlayer->SetVolume(m_volValue);
	}

	ui->appNameLabel->setStyleSheet("QLabel { color : white; }");

	if(m_pNxPlayer)
	{
        m_duration = m_pNxPlayer->GetMediaDuration();
	}

	if( (m_fileList.GetSize() == 0) || (false == m_bFindVideoFile) )
    {
        m_duration = 100 * 1000;    //dummy
    }
	durationChanged(m_duration);
	m_pTimer = new QTimer();
	//Update position timer
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(positionChanged()));
    //Update Subtitle
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(subTitleDisplayUpdate()));
	m_pTimer->start(300);

	setAttribute(Qt::WA_AcceptTouchEvents, true);
}

MainWindow::~MainWindow()
{
	if(m_pNxPlayer)
	{
        NX_MediaStatus state = m_pNxPlayer->GetState();
        if( (PlayingState == state)||(PausedState == state) )
        {
            stopVideo();
        }

		delete m_pNxPlayer;
		m_pNxPlayer = NULL;
	}

	if(m_pSubtitleParser)
	{
		delete m_pSubtitleParser;
		m_pSubtitleParser = NULL;
	}
	delete ui;
}

////////////////////////////////////////////////////////////////////
//
//      Update Player Progress Bar & Volume Progress Bar
//
////////////////////////////////////////////////////////////////////
void MainWindow::updateProgressBar(QMouseEvent *event, bool bReleased)
{
	int x = event->x();
	int y = event->y();

	//  Progress Bar Update
	QPoint pos = ui->progressBar->pos();
	int width = ui->progressBar->width();
	int height = ui->progressBar->height();
	int minX = pos.rx();
	int maxX = pos.rx() + width;
	int minY = pos.ry();
	int maxY = pos.ry() + height;

	if( bReleased )
	{
		if( (minX<=x && x<=maxX) && (minY<=y && y<=maxY) )
		{
			//	 Do Seek
			if( m_bSeekReady )
			{
                if( (StoppedState !=m_pNxPlayer->GetState()) && (ReadyState !=m_pNxPlayer->GetState()) )
                {
					double ratio = (double)(x-minX)/(double)width;
                    qint64 position = ratio * m_pNxPlayer->GetMediaDuration();
                    m_pNxPlayer->Seek( position );
					qDebug("Position = %lld", position);
				}
				qDebug("Do Seek !!!");
			}
		}
		m_bSeekReady = false;
	}
	else
	{
		if( (minX<=x && x<=maxX) && (minY<=y && y<=maxY) )
		{
			m_bSeekReady = true;
			qDebug("Ready to Seek\n");
		}
		else
		{
			m_bSeekReady = false;
		}
	}
}

void MainWindow::updateVolumeBar(QMouseEvent *event, bool bReleased)
{
	int x = event->x();
	int y = event->y();

	//  Progress Bar Update
	QPoint pos = ui->volumeProgressBar->pos();
	int width = ui->volumeProgressBar->width();
	int height = ui->volumeProgressBar->height();
	int minX = pos.rx();
	int maxX = pos.rx() + width;
	int minY = pos.ry();
	int maxY = pos.ry() + height;

	if( bReleased )
	{
		if( (minX<=x && x<=maxX) && (minY<=y && y<=maxY) )
		{
			//	 Change Volume
			if( m_bVoumeCtrlReady )
			{
                if( StoppedState !=m_pNxPlayer->GetState() )
				{
					double ratio = (double)(maxY-y)/(double)height;
					qint64 position = ratio * VOLUME_MAX;
					m_volValue = position;
					ui->volumeProgressBar->setValue(m_volValue);
					ui->volumelabel->setText(QString("%1").arg(m_volValue));
                    m_pNxPlayer->SetVolume(m_volValue);
					qDebug("Volume Value = %lld", position);
				}
				qDebug("Change Volume !!!");
			}
		}
		m_bVoumeCtrlReady = false;
	}
	else
	{
		if( (minX<=x && x<=maxX) && (minY<=y && y<=maxY) )
		{
			m_bVoumeCtrlReady = true;
			qDebug("Ready to Volume Control\n");
        }
		else
		{
			m_bVoumeCtrlReady = false;
		}
	}
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
	updateProgressBar(event, false);
	updateVolumeBar(event, false);
}


void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
	updateProgressBar(event, true);
	updateVolumeBar(event, true);
//    displayTouchEvent();
}

void MainWindow::on_prevButton_released()
{
	if(m_pNxPlayer)
	{
		if( (0 == m_fileIndex) || (1 == m_fileList.GetSize()) )
		{
			m_fileIndex = m_fileList.GetSize()-1;
			m_curFileListIdx = m_fileIndex;
		}
		else
		{
			m_fileIndex--;
			m_curFileListIdx = m_fileIndex;
		}
		stopVideo();
		playVideo();
	}
}

void MainWindow::on_playButton_released()
{
	if(m_pNxPlayer)
	{
		playVideo();
	}
}

void MainWindow::on_pauseButton_released()
{
	if(m_pNxPlayer)
	{
        m_pNxPlayer->Pause();
	}
}

void MainWindow::on_nextButton_released()
{
	if(m_pNxPlayer)
	{
		if( (m_fileList.GetSize()-1 == m_fileIndex) || ( 1 == m_fileList.GetSize() ) )
		{
			m_fileIndex = 0;
			m_curFileListIdx = m_fileIndex;
		}
		else
		{
			m_fileIndex++;
			m_curFileListIdx = m_fileIndex;
		}
		stopVideo();
		playVideo();
	}
}

void MainWindow::on_stopButton_released()
{
	if(m_pNxPlayer)
	{
		stopVideo();
		m_savePosition = 0;
	}
}

void MainWindow::durationChanged(qint64 duration)
{
	ui->progressBar->setValue(0);
	m_duration = duration/1000;
    //	ProgressBar
	ui->progressBar->setMaximum(duration / 1000);
	ui->progressBar->setRange(0, duration / 1000);
}

void MainWindow::positionChanged()
{
    if( (m_pNxPlayer) && (StoppedState != m_pNxPlayer->GetState()))
	{
        qint64 position = m_pNxPlayer->GetMediaPosition();
		//	Prgoress Bar
		ui->progressBar->setValue(position / 1000);
		updateDurationInfo(position / 1000);
	}
	else
	{
		ui->progressBar->setValue(0);
		updateDurationInfo(0);
	}
}

void MainWindow::updateDurationInfo(qint64 currentInfo)
{
	QString tStr;
	if (currentInfo || m_duration)
	{
		QTime currentTime((currentInfo/3600)%60, (currentInfo/60)%60, currentInfo%60, (currentInfo*1000)%1000);
		QTime totalTime((m_duration/3600)%60, (m_duration/60)%60, m_duration%60, (m_duration*1000)%1000);
		QString format = "mm:ss";
		if (m_duration > 3600)
		{
			format = "hh:mm:ss";
		}
		tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
	}
	ui->durationlabel->setText(tStr);
}

void MainWindow::statusChanged(int eventType)
{
	switch (eventType)
	{
		case MP_MSG_EOS:
		{
			qDebug("******** EndOfMedia !!!\n");

			ui->progressBar->setValue(0);
			updateDurationInfo(0);
			if(m_pNxPlayer)
			{
				if( (m_fileList.GetSize()-1 == m_fileIndex) || ( 1 == m_fileList.GetSize() ) )
				{
					m_fileIndex = 0;
					m_curFileListIdx = m_fileIndex;
				}
				else
				{
					m_fileIndex++;
					m_curFileListIdx = m_fileIndex;
				}
			}
			stopVideo();
			playVideo();

			break;
		}
		case MP_MSG_DEMUX_ERR:
		{
			qDebug("******** MP_MSG_DEMUX_ERR !!!\n");

			ui->progressBar->setValue(0);
			updateDurationInfo(0);
			if(m_pNxPlayer)
			{
				if( (m_fileList.GetSize()-1 == m_fileIndex) || ( 1 == m_fileList.GetSize() ) )
				{
					m_fileIndex = 0;
					m_curFileListIdx = m_fileIndex;
				}
				else
				{
					m_fileIndex++;
					m_curFileListIdx = m_fileIndex;
				}
			}
			stopVideo();
			playVideo();
			break;
		}
	}
}

void MainWindow::stopVideo()
{
	if( NULL == m_pNxPlayer)
		return;

    m_pNxPlayer->Stop();
    StopSubTitle();
    m_pNxPlayer->CloseHandle();
}

void MainWindow::playVideo()
{
	int ret = 0;

	if( NULL == m_pNxPlayer)
		return;

	if( 0 == m_fileList.GetSize())
		return;

    if( (PausedState == m_pNxPlayer->GetState()) || (ReadyState == m_pNxPlayer->GetState()))
	{
        m_pNxPlayer->Play();
		return;
	}
    else if(StoppedState == m_pNxPlayer->GetState())
	{
		while(true)
		{
            QString Uri = m_fileList.GetList(m_fileIndex);
            ret = m_pNxPlayer->InitMediaPlayer( cbEventCallback,
                                                NULL,
                                                Uri.toStdString().c_str(),
                                                MP_TRACK_VIDEO,
                                                NULL
                                               );
			if(ret < 0)
			{
                m_pNxPlayer->Stop();
                m_pNxPlayer->CloseHandle();

				if( (m_fileList.GetSize()-1 == m_fileIndex) || ( 1 == m_fileList.GetSize() ) )
				{
					m_fileIndex = 0;
					m_curFileListIdx = m_fileIndex;
				}
				else
				{
					m_fileIndex++;
					m_curFileListIdx = m_fileIndex;
				}
			}
			else
			{
                if( 0 == OpenSubTitle() )
                {
                    PlaySubTitle();
                }
				break;
			}
		}
	}

    m_duration = m_pNxPlayer->GetMediaDuration();
	durationChanged(m_duration);
    m_pNxPlayer->Play();
	ui->appNameLabel->setText(m_fileList.GetFileName(m_curFileListIdx));
    m_pNxPlayer->SetVolume(m_volValue);
}

void MainWindow::displayTouchEvent()
{
    if(false == m_bButtonHide)
	{
		m_bButtonHide = true;
		ui->progressBar->hide();
		ui->prevButton->hide();
		ui->playButton->hide();
		ui->pauseButton->hide();
		ui->nextButton->hide();
		ui->stopButton->hide();
		ui->playListButton->hide();
		ui->closeButton->hide();
		ui->volumelabel->hide();
		ui->volumeProgressBar->hide();
		ui->durationlabel->hide();
		ui->appNameLabel->hide();
		qDebug("**************** MainWindow:: Hide \n ");
	}
	else
	{
		qDebug("**************** MainWindow:: Show \n ");
		ui->progressBar->show();
		ui->prevButton->show();
		ui->playButton->show();
		ui->pauseButton->show();
		ui->nextButton->show();
		ui->stopButton->show();
		ui->playListButton->show();
		ui->closeButton->show();
		ui->volumelabel->show();
		ui->volumeProgressBar->show();
		ui->durationlabel->show();
		ui->appNameLabel->show();
		m_bButtonHide = false;
	}
}

void MainWindow::on_closeButton_released()
{
	stopVideo();
	this->close();
}


//
//		Play Util
//

void MainWindow::getAspectRatio(int srcWidth, int srcHeight,
                                int scrWidth, int scrHeight,
                                int *pWidth,  int *pHeight)
{
	// Calculate Video Aspect Ratio
	int dspWidth = 0, dspHeight = 0;
	double xRatio = (double)scrWidth / (double)srcWidth;
	double yRatio = (double)scrHeight / (double)srcHeight;

	if( xRatio > yRatio )
	{
		dspWidth    = (int)((double)srcWidth * yRatio);
		dspHeight   = scrHeight;
	}
	else
	{
		dspWidth    = scrWidth;
		dspHeight   = (int)((double)srcHeight * xRatio);
	}

	*pWidth     = dspWidth;
	*pHeight    = dspHeight;
}


//
//		Play List Button
//
void MainWindow::on_playListButton_released()
{
	PlayListWindow *pPlayList = new PlayListWindow();
	pPlayList->setWindowFlags(Qt::Window|Qt::FramelessWindowHint);

	pPlayList->setList(&m_fileList);
	pPlayList->setCurrentIndex(m_curFileListIdx);

	if( pPlayList->exec() )
	{
		qDebug("OK ~~~~~~");
		m_curFileListIdx = pPlayList->getCurrentIndex();
		m_fileIndex = m_curFileListIdx;
		on_stopButton_released();
		on_playButton_released();
	}
	else
	{
		qDebug("Cancel !!!!!");
	}
	delete pPlayList;
}

//
// Subtitle Display Routine
//

void MainWindow::subTitleDisplayUpdate()
{
    if(m_bSubThreadFlag)
    {
        if( (m_pNxPlayer) && (StoppedState != m_pNxPlayer->GetState()))
        {
            QString encResult;
            int idx;
            qint64 curPos = m_pNxPlayer->GetMediaPosition();
            for( idx = m_pNxPlayer->GetSubtitleIndex() ; idx <= m_pNxPlayer->GetSubtitleMaxIndex() ; idx++ )
            {
                if(m_pNxPlayer->GetSubtitleStartTime() < curPos)
                {
                    char *pBuf = m_pNxPlayer->GetSubtitleText();
                    encResult = m_pCodec->toUnicode(pBuf);

                    //HTML
                    //encResult = QString("%1").arg(m_pCodec->toUnicode(pBuf));	//&nbsp; not detected
                    //encResult.replace( QString("<br>"), QString("\n")  );		//detected
                    encResult.replace( QString("&nbsp;"), QString(" ")  );
                    if(m_bButtonHide == false)
                    {
                        ui->subTitleLabel->setText(encResult);
                    }
                    else
                    {
                        ui->subTitleLabel->setText("");
                    }
                }else
                {
                    break;
                }
                m_pNxPlayer->IncreaseSubtitleIndex();
            }
        }
    }
}


int MainWindow::OpenSubTitle()
{
    QString path;
    path = m_fileList.GetList(m_fileIndex);
    int lastIndex = path.lastIndexOf(".");
    char tmpStr[1024]={0};
    if((lastIndex == 0))
    {
        return -1;  //this case means there is no file that has an extension..
    }
    strncpy(tmpStr, (const char*)path.toStdString().c_str(), lastIndex);
    QString pathPrefix(tmpStr);
    QString subtitlePath;

    subtitlePath = pathPrefix + ".smi";

    //call library method
    int openResult = m_pNxPlayer->OpenSubtitle( (char *)subtitlePath.toStdString().c_str() );

    if ( 1 == openResult )
    {
        // case of open succeed
        m_pCodec = QTextCodec::codecForName(m_pNxPlayer->GetBestSubtitleEncode());

        if (NULL == m_pCodec)
            m_pCodec = QTextCodec::codecForName("EUC-KR");
        return 0;
    }else if( -1 == openResult )
    {
        //smi open tried but failed while fopen (maybe smi file does not exist)
        //should try opening srt
        subtitlePath = pathPrefix + ".srt";
        if( 1 == m_pNxPlayer->OpenSubtitle( (char *)subtitlePath.toStdString().c_str() ) )
        {
            m_pCodec = QTextCodec::codecForName(m_pNxPlayer->GetBestSubtitleEncode());
            if (NULL == m_pCodec)
                m_pCodec = QTextCodec::codecForName("EUC-KR");
            return 0;
        }else
        {
            //smi and srt both cases are tried, but open failed
            return -1;
        }
    }else
    {
        qDebug("parser lib OpenResult : %d\n",openResult);
        //other err cases
        //should check later....
        return -1;
    }

    return -1;
}

void MainWindow::PlaySubTitle()
{
    m_bSubThreadFlag = true;
}

void MainWindow::StopSubTitle()
{
    if(m_bSubThreadFlag)
    {
        m_bSubThreadFlag = false;
    }

    m_pNxPlayer->CloseSubtitle();

    ui->subTitleLabel->setText("");
}
