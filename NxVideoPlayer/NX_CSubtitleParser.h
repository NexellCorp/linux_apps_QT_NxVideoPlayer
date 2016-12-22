#ifndef NXSUBTITLEPARSER_H
#define NXSUBTITLEPARSER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <QString>
#include <QFile>
#include <NX_SubTitle.h>

enum
{
	TYPE_INVALID    = -1,
	TYPE_SMI        = 0,
	TYPE_SRT        = 1,
	TYPE_TXT        = 2
};


class NX_CSubtitleParser
{
public:
	NX_CSubtitleParser();
	~NX_CSubtitleParser();

public:
	int Open( QString path );
	void Close();
	char *Parse( int time );

private:
	int Search( int time );
	void AssParse(char *pAssBuf, int AssSize, char *pOutBuf, int outSize);

private:
	char    *m_pSubTitleData;
	int     m_TotalFrame;
	int     m_Index;
	void    *m_pHandle;
	char    m_subTitleBuf[1024];
	char    *m_pSubTitleLang;
};

#endif // NXSUBTITLEPARSER_H
