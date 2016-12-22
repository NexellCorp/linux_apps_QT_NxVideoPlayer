#include "NX_CSubtitleParser.h"
#include <qdebug.h>
#include <QTextCodec>

//------------------------------------------------------------------------------
NX_CSubtitleParser::NX_CSubtitleParser()
	:m_pSubTitleData(NULL)
	,m_TotalFrame(0)
	,m_Index(0)
	,m_pHandle(NULL)
{
}

//------------------------------------------------------------------------------
NX_CSubtitleParser::~NX_CSubtitleParser()
{
	if(m_pSubTitleData)
	{
		free(m_pSubTitleData);
		m_pSubTitleData = NULL;
	}

	if(m_pHandle)
	{
		NX_SubTitleClose(m_pHandle);
		m_pHandle = NULL;
	}
}

//------------------------------------------------------------------------------
int NX_CSubtitleParser::Open( QString path )
{
	int lastIndex = path.lastIndexOf(".");
	char tmpStr[1024]={0};
	if((lastIndex == 0))
	{
		return TYPE_INVALID;
	}
	strncpy(tmpStr, (const char*)path.toStdString().c_str(), lastIndex);
	QString pathPrefix(tmpStr);
	QString subtitlePath;
	QFile *pfileTemp;

	subtitlePath = pathPrefix + ".smi";
	pfileTemp = new QFile( subtitlePath );
	if( pfileTemp->exists() && (pfileTemp->size() > 0) )
	{
		char *pStart = NULL;
		char *pEnd = NULL;

		// Read
		QFile *pfIn = new QFile( subtitlePath );
		pfIn->open(QIODevice::ReadOnly);
		QString readStrFile(pfIn->readAll());

		pStart = (char*)strstr((const char *)readStrFile.toStdString().c_str(), ( const char *)"<HEAD>:");
		pEnd = (char*)strstr((const char *)readStrFile.toStdString().c_str(), ( const char *)"</HEAD>:");
		memcpy(tmpStr, pStart, pEnd - pStart);
		if(!strcmp((const char*)tmpStr, (const char*)"en-US"))
		{
			m_pSubTitleLang = (char *)"en-US";
		}
		else if(strcmp(tmpStr, (char *)"kr-KR"))
		{
			m_pSubTitleLang = (char *)"kr-KR";
		}
		else
		{
			m_pSubTitleLang = (char *)"en-US";
		}

		if(!strcmp((const char*)m_pSubTitleLang, (const char*)"kr-KR"))
		{

			// UTF-8 Convert
			QString writeString;
			QTextCodec * codec = QTextCodec::codecForName("eucKR");
			writeString = codec->toUnicode(readStrFile.toStdString().c_str());

			// Write
			subtitlePath = "/tmp/tmp.smi";
			QFile *pfOut = new QFile( subtitlePath );
			pfOut->open(QIODevice::WriteOnly);
			pfOut->write(writeString.toStdString().c_str(), writeString.length());
		}

		if( !NX_SubTitleOpen( &m_pHandle, (char *)subtitlePath.toStdString().c_str() ) )
		{
			int outSize = 0;
			char *pAssOutBuf = NULL;

			pAssOutBuf = (char *)malloc(pfileTemp->size());
			NX_SubTitleDecode( m_pHandle, pAssOutBuf, &outSize );

			if(m_pSubTitleData)
			{
				free(m_pSubTitleData);
				m_pSubTitleData = NULL;
			}
			m_pSubTitleData = (char *)malloc(pfileTemp->size());
			memset(m_pSubTitleData, 0, pfileTemp->size());
			AssParse(pAssOutBuf, outSize, m_pSubTitleData, 0);

			if(pAssOutBuf)
			{
				free(pAssOutBuf);
				pAssOutBuf = NULL;
			}

			NX_SubTitleClose(m_pHandle);
			m_pHandle = NULL;

			return TYPE_SMI;
		}
	}

	subtitlePath = pathPrefix + ".srt";
	pfileTemp = new QFile( subtitlePath );
	if( pfileTemp->exists() && (pfileTemp->size() > 0) )
	{
		if( !NX_SubTitleOpen( &m_pHandle, (char *)subtitlePath.toStdString().c_str() ) )
		{
			int outSize = 0;
			char *pAssOutBuf = NULL;

			pAssOutBuf = (char *)malloc(pfileTemp->size());
			NX_SubTitleDecode( m_pHandle, pAssOutBuf, &outSize );

			if(m_pSubTitleData)
			{
				free(m_pSubTitleData);
				m_pSubTitleData = NULL;
			}
			m_pSubTitleData = (char *)malloc(pfileTemp->size());
			memset(m_pSubTitleData, 0, pfileTemp->size());
			AssParse(pAssOutBuf, outSize, m_pSubTitleData, 0);

			if(pAssOutBuf)
			{
				free(pAssOutBuf);
				pAssOutBuf = NULL;
			}

			NX_SubTitleClose(m_pHandle);
			m_pHandle = NULL;

			return TYPE_SRT;
		}
	}

	return TYPE_INVALID;
}

//------------------------------------------------------------------------------
void NX_CSubtitleParser::Close()
{
	m_TotalFrame = 0;
	m_Index      = -1;

	if(m_pSubTitleData)
	{
		free(m_pSubTitleData);
		m_pSubTitleData = NULL;
	}
}

//------------------------------------------------------------------------------
// ass format
// Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
//

// out format
// totFrame|syncCode|frameSize|frameCount|startTime|endTime|textData|syncCode|frameSize|frameCount|startTime|endTime|textData|.....
//
void NX_CSubtitleParser::AssParse(char *pAssBuf, int AssSize, char *pOutBuf, int /*outSize*/)
{
	unsigned char *pAssStr = (unsigned char *)pAssBuf;
	unsigned char *pTmp = NULL;
	int remainSize = AssSize;
	unsigned char *pTmpAssStr= NULL;
	unsigned char *pTmpBuf= NULL;
	int32_t *p32_t = (int32_t *)pOutBuf;
	int frameCount = 0;

	pTmpBuf = (unsigned char *)malloc(1024*2);

	p32_t++;
	do{
		pTmpAssStr = pTmpBuf;
		memset(pTmpAssStr, 0, 1024*2);
		if(!strncmp((const char *)pAssStr, (const char *)"Dialogue:", 9))   //Format
		{
			int assFrameSize = 0;
			int32_t startTime = 0;
			int32_t endTime = 0;
			int32_t h =0, m = 0;
			float s = 0;


			pTmp = pAssStr;
			pTmp++;
			pTmp = (unsigned char*)strstr((const char *)pTmp, ( const char *)"Dialogue:"); //next Dialogue
			if(pTmp == NULL)
			{
				break;
			}
			assFrameSize = pTmp - pAssStr;
			remainSize = remainSize - assFrameSize;
			if(remainSize <= 0)
			{
				break;
			}
			memcpy(pTmpAssStr, pAssStr, assFrameSize);
			pAssStr = pAssStr + assFrameSize;


			pTmp = NULL;
			pTmp = (unsigned char*)strchr((const char*)pTmpAssStr,',');         //Layer
			assFrameSize = assFrameSize - (pTmp - pTmpAssStr);
			pTmpAssStr = pTmp;

			pTmpAssStr++;                          //Start Time
			assFrameSize--;
			sscanf((const char*)pTmpAssStr,"%d:%d:%f",&h,&m,&s);
			startTime = (int32_t)(s * 1000.0);
			startTime = startTime + ((m * 60) * 1000);
			startTime = startTime + ((h * 60 * 60) * 1000);
			pTmp = NULL;
			pTmp = (unsigned char*)strchr((const char*)pTmpAssStr,',');
			assFrameSize = assFrameSize - (pTmp - pTmpAssStr);
			pTmpAssStr = pTmp;

			pTmpAssStr++;                      //End Time
			assFrameSize--;
			sscanf((const char*)pTmpAssStr,"%d:%d:%f",&h,&m,&s);
			endTime = (int32_t)(s * 1000.0);
			endTime = endTime + ((m * 60) * 1000);
			endTime = endTime + ((h * 60 * 60) * 1000);
			pTmp = NULL;
			pTmp = (unsigned char*)strchr((const char*)pTmpAssStr,',');
			assFrameSize = assFrameSize - (pTmp - pTmpAssStr);
			pTmpAssStr = pTmp;

			// Style, Name, MarginL, MarginR, MarginV, Effect
			for(int i = 0; i < 6; i++)
			{
				pTmpAssStr++;
				assFrameSize--;
				pTmp = NULL;
				pTmp = (unsigned char*)strchr((const char*)pTmpAssStr,',');
				assFrameSize = assFrameSize - (pTmp - pTmpAssStr);
				pTmpAssStr = pTmp;
			}

			pTmpAssStr++;                      //Text
			assFrameSize--;

			char *pChar = NULL;
			frameCount++;
			*p32_t++ = 0xfffffffb;						//sync code
			*p32_t++ = (4*3) +  assFrameSize;			//Frame Size: Frame Count + StartTime + EndTime  + Text Dats
			*p32_t++ = frameCount;
			*p32_t++ = startTime;
			*p32_t++ = endTime;

			const char *pFindStr = "\\N";
			const char *pResultStr = NULL;
			pResultStr = strstr((const char *)pTmpAssStr,pFindStr);
			if(pResultStr)
			{
				int32_t len = strlen((const char*)pTmpAssStr);
				int32_t alen = (unsigned char*)pResultStr - pTmpAssStr;
				pChar = (char *)p32_t;
				strncpy(pChar, (const char*)pTmpAssStr, alen);
				pChar = pChar + alen;
				pFindStr = "\n";
				pResultStr = strncpy(pChar,pFindStr,2);
				pChar = pChar + 2;
				pTmpAssStr = pTmpAssStr + alen + 2;
				strncpy(pChar, (const char*)pTmpAssStr, len - alen-2);
				pChar = pChar + (len - alen - 2);
				p32_t = (int32_t *)pChar;
			}
			else
			{
				strncpy((char *)p32_t, (const char*)pTmpAssStr, assFrameSize);
				pChar = (char *)p32_t;
				pChar = pChar + assFrameSize;
				p32_t = (int32_t *)pChar;
			}
		}


	}while(1);

	p32_t = (int32_t *)pOutBuf;
	*p32_t = frameCount;

	if(pTmpBuf)
	{
		free(pTmpBuf);
	}
}

//------------------------------------------------------------------------------
int NX_CSubtitleParser::Search( int time )
{
	int index = 0, syncCode, frameSize;
	int startTime, endTime;

	m_TotalFrame = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
	index = index + 4;

	for( int i = 0; i < m_TotalFrame; i++ )
	{
		syncCode = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
		index += 4;
		if((unsigned int)syncCode == 0xFFFFFFFB)
		{
			frameSize   = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
			index += 8;
			startTime   = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
			index += 4;
			endTime     = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
			index += 4;

			if( (time >= startTime) && (time <= endTime) )
			{
				m_Index      = index - (5 * 4);
				return 0;
			}
			index = index + frameSize - (3 * 4);
		}
	}
	return -1;
}

//------------------------------------------------------------------------------
char *NX_CSubtitleParser::Parse( int time )
{
	int index, frameSize;
	//    int startTime, endTime;

	if( 0 > Search( time ) )
		return (char *)"";

	index = m_Index;
	index += 4;
	frameSize   = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
	index += 8;
	//    startTime   = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
	index += 4;
	//    endTime     = ((m_pSubTitleData[index + 3] << 24)) | ((0x00FFFFFF) & (m_pSubTitleData[index + 2] << 16)) | ((0x0000FFFF) & (m_pSubTitleData[index + 1] << 8)) | ((0x000000FF) & (m_pSubTitleData[index]));
	index += 4;

	memset(m_subTitleBuf, 0, 1024);
	memcpy(m_subTitleBuf, m_pSubTitleData + index, frameSize - (3 * 4));

	return m_subTitleBuf;
}
