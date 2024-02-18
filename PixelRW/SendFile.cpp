#include "pch.h"
#include "SendFile.h"

CSendFile::CSendFile(CPixelRWDlg* dlg, CRect& rect) :CBase(dlg) {
	m_rect = rect;
	m_nBufSize = m_rect.Width() * m_rect.Height() * 4;
	m_pBuf = new BYTE[m_nBufSize * SPLIT_COUNT * SPLIT_COUNT];
	ZeroMemory(m_pBuf, m_nBufSize * SPLIT_COUNT * SPLIT_COUNT);

	EmptyClipboard();
	TCHAR tchBuf[200];
	_stprintf_s(tchBuf, 200, _T("RECT Left top, right bottom %d %d %d %d\n"), m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
	m_dlg->Log(tchBuf);

	m_desktop_ctx.hWnd = ::GetDesktopWindow();

	//m_desktop_ctx.hWnd = FindVmWndHandle(_T("Windows 11 x64 - VMware Workstation"));

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
	int ret = 0;
	int32_t nId = -1;

	if (IsDataWritable(CONNECTION_WAIT_TIMEOUT, nId) == 0)
	{
		CFile file;
		if (file.Open(pctszFileName, CFile::modeRead))
		{
			uint64_t nRemainder = 0;
			file_info_t file_info = { 0 };

			file_info.nFileSize = file.GetLength();
			file_info.nFileCheckSum = GetFileCheckSum(&file);
			_tcscpy_s(file_info.tchFileName, MAX_PATH, pctszFileName);

			file_info.fh.nId = nId;
			file_info.fh.nDataSize = sizeof(file_info_t) - sizeof(frame_header_t);
			file_info.fh.nCheckSum = (int32_t)CalCheckSum((BYTE*)&file_info + sizeof(frame_header_t), file_info.fh.nDataSize);

			nRemainder = file_info.nFileSize;
			m_dlg->Log(_T("Send data block id:%d, DataSize:%d, CheckSum:%d, remainder:%lldKB."), file_info.fh.nId, file_info.fh.nDataSize, file_info.fh.nCheckSum, nRemainder / 1024);
			m_dlg->Log(_T("Send file : %s, File size:%lld, file check sum:%lld."), file_info.tchFileName, file_info.nFileSize, file_info.nFileCheckSum);

			SetDataToScreenBuf((BYTE*)&file_info, sizeof(file_info_t));

			uint32_t nMaxReadDataLen = m_nBufSize / 4 * 3 * SPLIT_COUNT * SPLIT_COUNT;
			if (nMaxReadDataLen < sizeof(frame_header_t)) nMaxReadDataLen = sizeof(frame_header_t);
			BYTE* readBuf = new BYTE[nMaxReadDataLen];

			do
			{
				uint64_t nFilePos;
				ret = IsDataWritable(RW_WAIT_TIMEOUT, nId + 1, &nFilePos);
				if (ret == 0)
				{
					if (nId == -1)
					{
						file.Seek(nFilePos, CFile::begin);
					}
						
					frame_header_t fh;
					fh.nDataSize = file.Read(readBuf + sizeof(frame_header_t), nMaxReadDataLen - sizeof(frame_header_t));
					fh.nCheckSum = (uint32_t)CalCheckSum(readBuf + sizeof(frame_header_t), fh.nDataSize);
					nId++;
					fh.nId = nId;
					memcpy(readBuf, (void*)&fh, sizeof(frame_header_t));

					SetDataToScreenBuf(readBuf, fh.nDataSize + sizeof(frame_header_t));
					nRemainder -= fh.nDataSize;
					m_dlg->Log(_T("Send data block id:%d, DataSize:%d, CheckSum:%d, remainder:%lldKB."), fh.nId, fh.nDataSize, fh.nCheckSum, nRemainder / 1024);
				}
				else if (ret == RET_ERROR)
				{
					m_dlg->Log(_T("Send file is stopped!"));
					break;
				}
				else if (ret == RET_COMPLETE)
				{
					m_dlg->Log(_T("Send file complete!"));
					break;
				}
				else//timeout
				{
					m_dlg->Log(_T("Send file data time out!"));
					break;
				}

				if (m_dlg->IsAbort())
				{
					ret = -1;
					break;
				}
			} while (true);

			delete[] readBuf;
			file.Close();
		}
		else
		{
			ret = -1;
			m_dlg->Log(_T("file open error!"));
		}
	}
	else
	{
		m_dlg->Log(_T("Connect wait timeout! IsDataWritable"));
		ret = -1;
	}
	::InvalidateRect(NULL, m_rect, TRUE);

	return ret;
}

uint64_t CSendFile::GetFilePos(TCHAR* buf)
{
	TCHAR* ret = _tcsrchr(buf, _T(','));
	uint64_t nPos = _ttoi(ret + 1);
	return nPos;
}

int CSendFile::IsDataWritable(uint32_t timeout, int32_t nId, uint64_t *pnFilePos) 
{
	//m_dlg->Log(_T("IsDataWritable begin %d"), nId);
	int ret = RET_TIMEOUT;
	TCHAR buf[100] = {0};
	time_t oldTime = time(NULL);
	while ((time(NULL) - oldTime) < timeout)
	{
		if (nId == -1)
			FillRectRed();
		else
			WriteDataToScreen();
	
		if (GetTextFromClipboard(buf, 100))
		{
			if (IsContinue(buf, nId))
			{
				if (nId == 0 && pnFilePos)
					*pnFilePos = GetFilePos(buf);
				ret = 0;
				break;
			}
			else if (IsComplete(buf))
			{
				ret = RET_COMPLETE;
				break;
			}
			else if (IsRetry(buf))
			{
				m_dlg->Log(_T("Get RETRY from receive"));
			}
			else if (IsError(buf))
			{
				m_dlg->Log(_T("Get ERROR from receive"));
				ret = RET_ERROR;
				break;
			}
		}
		Sleep(20);
	};

	if (ret == RET_TIMEOUT) m_dlg->Log(_T("IsDataWritable timeout, buf: %s!"), buf);
	return ret;
}

void CSendFile::WriteDataToScreen() 
{
	//BYTE buf[4];
	//buf[0] = m_pBuf[0];
	//buf[1] = m_pBuf[1];
	//buf[2] = m_pBuf[2];
	//buf[3] = m_pBuf[4];
	//m_dlg->Log(_T("WriteDataToScreen, fh id: %d"), *((int32_t*)buf));

	for (int32_t i = 0; i < SPLIT_COUNT; i++)
	{
		for (int32_t j = 0; j < SPLIT_COUNT; j++)
		{
			HBITMAP MyBit = ::CreateCompatibleBitmap(m_desktop_ctx.hdc, m_rect.Width(), m_rect.Height());
			LONG ret = ::SetBitmapBits(MyBit, m_nBufSize, m_pBuf + m_nBufSize * (i * SPLIT_COUNT + j));
			if (ret)
			{
				HGDIOBJ oldBitmap = ::SelectObject(m_desktop_ctx.hdcMem, MyBit);
				::BitBlt(m_desktop_ctx.hdc, m_rect.left + i * m_rect.Width(), m_rect.top + j * m_rect.Height(), 
					m_rect.Width(), m_rect.Height(), m_desktop_ctx.hdcMem, 0, 0, SRCCOPY);
				::SelectObject(m_desktop_ctx.hdcMem, oldBitmap);
				::DeleteObject(MyBit);
			}
			else
				m_dlg->Log(_T("WriteDataToScreen Error"));
		}
	}

	//HBITMAP MyBit = ::CreateCompatibleBitmap(m_desktop_ctx.hdc, m_rect.Width(), m_rect.Height());
	//LONG ret = ::SetBitmapBits(MyBit, m_nBufSize, m_pBuf);
	//if (ret)
	//{
	//	HGDIOBJ oldBitmap = ::SelectObject(m_desktop_ctx.hdcMem, MyBit);
	//	::BitBlt(m_desktop_ctx.hdc, m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), m_desktop_ctx.hdcMem, 0, 0, SRCCOPY);

	//	::SelectObject(m_desktop_ctx.hdcMem, oldBitmap);
	//	::DeleteObject(MyBit);
	//}
	//else
	//	m_dlg->Log(_T("WriteDataToScreen Error"));
}

bool CSendFile::SetDataToScreenBuf(BYTE* pData, uint32_t nDataSize) const
{
	//m_dlg->Log(_T("SetDataToScreenBuf: datasize:%d"), nDataSize);
	if (pData == NULL) return false;
	if (m_nBufSize * SPLIT_COUNT * SPLIT_COUNT < (nDataSize + 2) / 3 * 4)	return false;

	uint32_t src = 0, dst = 0;
	for (; src < nDataSize;)
	{
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = pData[src++];
		m_pBuf[dst++] = (BYTE)0x0;
	}

	while(dst < m_nBufSize)
		m_pBuf[dst++] = 0;

	return true;
}

int CSendFile::FillRectRed() 
{
	int ret = 0;

	CRect rect = m_rect;
	FillRect(m_desktop_ctx.hdc, &rect, CBrush(RGB(255, 0, 0)));
	
	return ret;
}

bool CSendFile::IsContinue(LPCTSTR pctszText, int nId) const
{
	bool ret = false;
	if (_tcsncmp(pctszText, REQUEST_CONTINUE, _tcslen(REQUEST_CONTINUE)) == 0)
	{
		int num = _ttoi(pctszText + _tcslen(REQUEST_CONTINUE));
		if (nId == num)
		{
			m_dlg->Log(_T("Receive %s"), pctszText);
			ret = true;
		}
	}
	return ret;
}

bool CSendFile::IsError(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REQUEST_ERROR) == 0)
	{
		m_dlg->Log(_T("Receive %s"), pctszText);
		ret = true;
	}
	
	return ret;
}

bool CSendFile::IsComplete(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REQUEST_COMPLETE) == 0)
	{
		m_dlg->Log(_T("Receive %s"), pctszText);
		ret = true;
	}
	return ret;
}

bool CSendFile::IsRetry(LPCTSTR pctszText) const
{
	bool ret = false;
	if (_tcscmp(pctszText, REQUEST_RETRY) == 0)
	{
		m_dlg->Log(_T("Receive %s"), pctszText);
		ret = true;
	}
	return ret;
}