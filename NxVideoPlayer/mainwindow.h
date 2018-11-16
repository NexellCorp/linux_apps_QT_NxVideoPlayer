#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QMediaPlayer>
#include <QTime>
#include <QTimer>
#include <QDebug>
#include <QTouchEvent>

#include "NX_CFileList.h"
#include "CNX_MoviePlayer.h"
#include "CNX_SubtitleParser.h"

#include "CNX_Util.h"
#include "playlistwindow.h"


#define VOLUME_MIN  0
#define VOLUME_MAX  100

#include <QObject>
class CallBackSignal : public QObject
{
	Q_OBJECT

public:
	CallBackSignal() {}

public slots:
	void statusChanged(int eventType)
	{
		emit mediaStatusChanged(eventType);
	}

signals:
	void mediaStatusChanged(int newValue);
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	//
	//	Mouse Event
	//
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void getAspectRatio(int srcWidth, int srcHeight,
						int scrWidth, int scrHeight,
						int *pWidth, int *pHeight);
	void setResize(int dspStatus);
	void ImageUpdate(void *pImg);
	void displayTouchEvent();

	//
	//	SubTitle
	//
    int OpenSubTitle();
    void PlaySubTitle();
    void StopSubTitle();


	//
	//	Play
	//
	void stopVideo();
	void playVideo();

private:
	qint64 m_duration;
	qint64 m_savePosition;
	int    m_volValue;
	int    m_fileIndex;
    CNX_MoviePlayer *m_pNxPlayer;
	NX_CFileList    m_fileList;
	QTimer          *m_pTimer;
	int             m_DspMode;
	bool			m_bFindVideoFile;
	//	Progress Bar
	bool	m_bSeekReady;
	bool    m_bVoumeCtrlReady;
	bool    m_bButtonHide;
	int     m_curFileListIdx;
	int     m_scrWidth;
	int     m_scrHeight;

	// Subtitle
    CNX_SubtitleParser  *m_pSubtitleParser;
	bool                m_bSubThreadFlag;
	QTimer              *m_pSubTitleTimer;
    QTextCodec*         m_pCodec;

private:
	void updateDurationInfo(qint64 currentInfo);
	void durationChanged(qint64 duration);

	//  Update Progress Bar
private:
	void updateProgressBar(QMouseEvent *event, bool bReleased);
	void updateVolumeBar(QMouseEvent *event, bool bReleased);

private slots:
	void subTitleDisplayUpdate();
	void statusChanged(int eventType);
	void positionChanged();

	void on_prevButton_released();
	void on_playButton_released();
	void on_pauseButton_released();
	void on_nextButton_released();
	void on_stopButton_released();

	//	Playlist Button & Close Button
	void on_closeButton_released();
	void on_playListButton_released();

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
