#include "pch.h"
#include "SendFile.h"

CSendFile::CSendFile(CPixelRWDlg* dlg, CRect& rect) :CBase(dlg) {
	m_rect = rect;
	m_nBufSize = m_rect.Width() * m_rect.Height() * 4;
	m_pBuf = new BYTE[m_nBufSize];
	ZeroMemory(m_pBuf, m_nBufSize);

	EmptyClipboard();
	TCHAR tchBuf[200];
	_stprintf_s(tchBuf, 200, _T("RECT Left top, right bottom %d %d %d %d\n"), m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
	m_dlg->Log(tchBuf);

	m_desktop_ctx.hWnd = ::GetDesktopWindow();

	m_desktop_ctx.hdc = NULL;
	m_desktop_ctx.hdcMem = NULL;

	InitDC();
}

CSendFile::~CSendFile()
{ 
	DeinitDC();
	if (m_pBuf) delete[] m_pBuf; 
}

void CSendFile::InitDC()
{
	DeinitDC();
	m_desktop_ctx.hdc = ::GetDC(m_desktop_ctx.hWnd);
	m_desktop_ctx.hdcMem = ::CreateCompatibleDC(m_desktop_ctx.hdc);
}

void CSendFile::DeinitDC()
{
	if (m_desktop_ctx.hdcMem) {
		::DeleteObject(m_desktop_ctx.hdcMem);
		m_desktop_ctx.hdcMem = NULL;
	}

	if (m_desktop_ctx.hdc) {
		::ReleaseDC(m_desktop_ctx.hWnd, m_desktop_ctx.hdc);
		m_desktop_ctx.hdc = NULL;

	}
}

int CSendFile::SendFile(LPCTSTR pctszFileName)
{
	int ret = -1;

	FillRectRed();
	if (IsDataWritable(CONNECTION_WAIT_TIMEOUT) == 0)
	{
		CFile file;
		m_dlg->Log(_T("before file open"));
		if (file.Open(pctszFileName, CFile::modeRead))
		{
			uint64_t nRemainder = 0;
			uint32_t nId = 0;

			// send file size(8Bytes), checksum value(8Bytes) and file name, and make the last line red.
			file_info_t file_info = { 0 };

			file_info.filesize = file.GetLength();
			file_info.checksum = GetFileCheckSum(&file);
			_tcscpy_s(file_info.filename, MAX_PATH, pctszFileName);

			file_info.fh.nId = nId++;
			file_info.fh.nDataSize = sizeof(file_info_t) - sizeof(frame_header_t);
			file_info.fh.nCheckSum = (int32_t)CalCheckSum((BYTE*)&file_info + sizeof(frame_header_t), file_info.fh.nDataSize);

			nRemainder = file_info.filesize;

			m_dlg->Log(_T("Send file : %s, File size:%lld, check sum:%lld."), file_info.filename, file_info.filesize, file_info.checksum);

			uint32_t nMaxReadDataLen = m_nBufSize / 4 * 3;
			BYTE* readBuf = new BYTE[nMaxReadDataLen];

			SetDataToScreenBuf((BYTE*)&file_info, sizeof(file_info_t));

			file.SeekToBegin();
			m_dlg->Log(_T("Send file begin."));
			do
			{
				UINT nRetry = 1;
				int ret1 = 0;
				do
				{
					if (m_dlg->IsAbort())
					{
						ret1 = -1;
						break;
					}

					WriteDataToScreen();

					nRetry++;

					if (nRemainder == 0)
					{
						m_dlg->Log(_T("Send file end."));
						ret = 0;
						break;
					}
					ret1 = IsDataWritable(RW_WAIT_TIMEOUT);
					if (ret1 == 0)
					{
						break;
					}
					else if (ret1 == RET_RETRY && nRetry < MAX_RETRY_TIMES)
					{
						m_dlg->Log(_T("Send file data, Retry:%d"), nRetry);
						continue;
					}
					else if (ret1 == RET_ERROR)
					{
						ret1 = -1;
						m_dlg->Log(_T("Send file is stopped!"));
						break;
					}
					else
					{
						m_dlg->Log(_T("Send file data retry out!"));
						ret1 = -1;
						break;
					}
				} while (true);
				if (ret1 == -1) break;
				if (ret == 0) break;

				frame_header_t fh;
				fh.nDataSize = file.Read(readBuf + sizeof(frame_header_t), nMaxReadDataLen - sizeof(frame_header_t));
				fh.nCheckSum = (uint32_t)CalCheckSum(readBuf + sizeof(frame_header_t), fh.nDataSize);
				fh.nId = nId++;
				memcpy(readBuf, (void*)&fh, sizeof(frame_header_t));

				SetDataToScreenBuf(readBuf, fh.nDataSize + sizeof(frame_header_t));
				nRemainder -= fh.nDataSize;
				m_dlg->Log(_T("Sending file data id:%d, DataSize:%d, CheckSum:%d, remainder:%lldKB."), fh.nId, fh.nDataSize, fh.nCheckSum, nRemainder / 1024);
			} while (true);

			delete[] readBuf;
			file.Close();
		}
		else
			m_dlg->Log(_T("file open error!"));
	}
	else
	{
		m_dlg->Log(_T("Connect wait timeout! IsDataWritable"));
		::InvalidateRect(NULL, m_rect, TRUE);
	}
	return ret;
}

int CSendFile::IsDataWritable(uint32_t timeout) 
{
	//m_dlg->Log(_T("IsDataWritable begin"));
	int ret = RET_TIMEOUT;
	time_t oldTime = time(NULL);
	while ((time(NULL) - oldTime) < timeout)
	{
		TCHAR buf[100];

		if (!GetTextFromClipboard(buf, 100, FALSE)) 
		{
			Sleep(5); 
			continue;
		}
		//m_dlg->Log(_T("IsDataWritable get text %s."), buf);

		if (IsContinue(buf))
		{
			m_dlg->Log(_T("Get CONTINUE from receive"));
			ret = 0;
			break;
		}
		else if (IsRetry(buf))
		{
			m_dlg->Log(_T("get RETRY from receive"));
			ret = RET_RETRY;
			break;
		}
		else if (IsError(buf))
		{
			m_dlg->Log(_T("get ERROR from receive"));
			ret = RET_ERROR;
			break;
		}
		else
		{
			Sleep(5);
		}
	};

	if (ret == RET_TIMEOUT)
	{
		ret = RET_RETRY;
		m_dlg->Log(_T("IsDataWritable timeout!"));
	}

	EmptyClipboard();

	//m_dlg->Log(_T("IsDataWritable end"));
	return ret;
}

void CSendFile::WriteDataToScreen() 
{
	BYTE buf[100];
	for (uint32_t src = 0, dst = 0; src < 30;)
	{
		buf[dst++] = m_pBuf[src++];
		buf[dst++] = m_pBuf[src++];
		buf[dst++] = m_pBuf[src++];
		src++;
	}

	frame_header_t* fh = (frame_header_t*)buf;
	m_dlg->Log(_T("WriteDataToScreen write img begin, fh.id:%d"), fh->nId);

	HBITMAP MyBit = ::CreateCompatibleBitmap(m_desktop_ctx.hdc, m_rect.Width(), m_rect.Height());
	LONG ret = ::SetBitmapBits(MyBit, m_nBufSize, m_pBuf);
	if (ret)
	{
		HGDIOBJ oldBitmap = ::SelectObject(m_desktop_ctx.hdcMem, MyBit);
		::BitBlt(m_desktop_ctx.hdc, m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), m_desktop_ctx.hdcMem, 0, 0, SRCCOPY);

		::SelectObject(m_desktop_ctx.hdcMem, oldBitmap);
		::DeleteObject(MyBit);
	}
	else
		m_dlg->Log(_T("WriteDataToScreen Error"));

	//below is not necessary
	//CString str = _T("WriteDataToScreen ");
	//for (size_t i = 0; i < 20*20*4; i++)
	//{
	//	CString s;
	//	if(i%4 != 3) s.Format(_T("%02X "), m_pBuf[i]);
	//	str += s;
	//}
	//m_dlg->Log(str);

	for (uint32_t src = 0, dst = 0; src < 30;)
	{
		buf[dst++] = m_pBuf[src++];
		buf[dst++] = m_pBuf[src++];
		buf[dst++] = m_pBuf[src++];
		src++;
	}
	fh = (frame_header_t*)buf;
	m_dlg->Log(_T("WriteDataToScreen write img end, fh.id:%d"), fh->nId);
}

bool CSendFile::SetDataToScreenBuf(BYTE* pData, uint32_t nDataSize) const
{
	m_dlg->Log(_T("SetDataToScreenBuf: datasize:%d"), nDataSize);
	if (pData == NULL) return false;
	if (m_nBufSize < (nDataSize + 2) / 3 * 4)	return false;

	for (uint32_t src = 0, dst = 0; src < nDataSize;)
	{
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = (BYTE)0x0;
	}
	return true;
}

uint64_t CSendFile::GetFileCheckSum(CFile* file) const
{
	uint64_t nCheckSum = 0;
	BYTE buf[4096];
	file->SeekToBegin();
	while (1)
	{
		UINT nCount = file->Read(buf, 4096);
		nCheckSum += CalCheckSum(buf, nCount);
		if (nCount < 4096) break;
	};
	return nCheckSum;
}

int CSendFile::FillRectRed() 
{
	int ret = 0;

	CRect rect = m_rect;
	FillRect(m_desktop_ctx.hdc, &rect, CBrush(RGB(255, 0, 0)));
	
	return ret;
}

bool CSendFile::IsContinue(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REPLY_CONTINUE) == 0)
		ret = true;
	return ret;
}
bool CSendFile::IsRetry(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REPLY_RETRY) == 0)
		ret = true;
	return ret;

}
bool CSendFile::IsError(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REPLY_ERROR) == 0)
		ret = true;
	return ret;
}
