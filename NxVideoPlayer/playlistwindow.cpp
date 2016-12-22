#include "playlistwindow.h"
#include "ui_playlistwindow.h"

#include <QScrollBar>

PlayListWindow::PlayListWindow(QWidget *parent) :
QDialog(parent),
	m_pretIdx(-1),
	m_selectCount(0),
	ui(new Ui::PlayListWindow)
{
	ui->setupUi(this);

	memset(&m_selectIdx,0,sizeof(QModelIndex) );

	ui->listWidget->setStyleSheet(
		"QListWidget{"
		"padding-background : white;"
		"padding-left : 20px;"
		"}"
		"QListWidget::item {"
		"padding : 20px;"
		"}");

	ui->listWidget->verticalScrollBar()->setStyleSheet(
		"QScrollBar:vertical{ width : 50px; }"
		);
}

PlayListWindow::~PlayListWindow()
{
	delete ui;
}

//
//	List Index Control
//
void PlayListWindow::setList(NX_CFileList *pFileList)
{
	for( int i=0 ; i<pFileList->m_FileList.size() ; i++ )
	{
		ui->listWidget->addItem(pFileList->m_FileNameList[i]);
	}
}

void PlayListWindow::setCurrentIndex(int idx)
{
	ui->listWidget->setCurrentRow(idx);
}

int PlayListWindow::getCurrentIndex()
{
	return m_selectIdx.row();
}

//
//	Button Event Control
//


void PlayListWindow::on_btnCancel_released()
{
	this->reject();
}

void PlayListWindow::on_btnOk_released()
{
	m_selectIdx = ui->listWidget->currentIndex();
	qDebug("on_btnOk_released : row, index : %d", m_selectIdx.row() );
	this->accept();
}

void PlayListWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
	m_selectCount++;
	m_selectIdx = item->listWidget()->currentIndex();
	if(2 == m_selectCount)
	{
		if(m_pretIdx == m_selectIdx.row())
		{
			this->accept();
		}
	}
	else
	{
		m_selectCount = 1;
	}
	m_pretIdx = m_selectIdx.row();
}
