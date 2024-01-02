#include "pch.h"
#include "ReceiveFile.h"

CReceiveFile::CReceiveFile(CPixelRWDlg* dlg, CRect& rect) :CBase(dlg) {
	m_rect = rect;

	m_nBufSize = m_rect.Width() * m_rect.Height() * 4;
	m_pBuf = new BYTE[m_nBufSize];

	m_ctx.hdc = NULL;
	m_ctx.hdcMem = NULL;

	m_ctx.hWnd = ::GetDesktopWindow();
	//#if LOCAL_COPY
	//	m_ctx.hWnd = ::GetDesktopWindow();
	//#else
	//	m_ctx.hWnd = FindVmWndHandle(_T("华为云客户端 - WANGY1"));
	//#endif

	InitDC();
}

CReceiveFile::~CReceiveFile() {
	Disconnect();
	if (m_pBuf) delete[] m_pBuf;
}

bool CReceiveFile::IsRightOK(int x1, int y1, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	bool ret = true;

	BYTE* row = &pRGBBuf[y1 * nWidth * 3];
	for (int32_t x = x1; x < x1+ MIN_RECT_WIDTH; x++)
	{
		if (!(row[3*x] == 0x0 && row[3*x + 1] == 0 && row[3*x + 2] == 0xFF))
		{
			ret = false;
			break;
		}
	}
	return ret;
}

bool CReceiveFile::IsLeftOK(int x1, int y1, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	bool ret = true;

	BYTE* row = &pRGBBuf[y1 * nWidth * 3];
	for (int32_t x = x1; x > x1 - MIN_RECT_WIDTH; x--)
	{
		if (!(row[3 * x] == 0 && row[3 * x + 1] == 0 && row[3 * x + 2] == 0xFF))
		{
			ret = false;
			break;
		}
	}
	return ret;
}

bool CReceiveFile::IsUpOK(int x1, int y1, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	bool ret = true;

	for (int32_t y = y1; y > y1 - MIN_RECT_HEIGHT; y --)
	{
		BYTE* row = &pRGBBuf[(y * nWidth + x1) * 3];
		if (!(row[0] == 0 && row[1] == 0 && row[2] == 0xFF))
		{
			ret = false;
			break;
		}
	}
	return ret;
}

bool CReceiveFile::IsDownOK(int x1, int y1, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	bool ret = true;

	for (int32_t y = y1; y < y1 + MIN_RECT_HEIGHT; y ++)
	{
		BYTE* row = &pRGBBuf[(y * nWidth + x1) * 3];
		if (!(row[0] == 0 && row[1] == 0 && row[2] == 0xFF))
		{
			ret = false;
			break;
		}
	}
	return ret;
}

 int CReceiveFile::GetLeftTop(POINT *point, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	int ret = -1;

	for (uint32_t y = 0; y < nHeight - MIN_RECT_HEIGHT; y++)
	{
		BYTE* row = &pRGBBuf[y * nWidth * 3];
		for (uint32_t x = 0; x < nWidth - MIN_RECT_WIDTH; x ++)
		{
			if (row[3 * x] == 0 && row[3 * x + 1] == 0 && row[3 * x + 2] == 0xFF)
			{
				if (IsRightOK(x, y, nWidth, nHeight, pRGBBuf, nRGBBufSize) && IsDownOK(x, y, nWidth, nHeight, pRGBBuf, nRGBBufSize))
				{
					point->x = x;
					point->y = y;
					ret = 0;
					break;
				}
			}
		}
		if (ret == 0) break;
	}
	return ret;
}

int CReceiveFile::GetRightBottom(POINT* point, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const
{
	int ret = -1;
	for (int32_t y = nHeight - 1; y >= MIN_RECT_HEIGHT; y--)
	{
		BYTE* row = &pRGBBuf[y * nWidth * 3];
		for (int32_t x = nWidth - 1; x >= MIN_RECT_WIDTH; x--)
		{
			if (row[3*x] == 0 && row[3 * x + 1] == 0 && row[3 * x + 2] == 0xFF)
			{
				if (IsLeftOK(x, y, nWidth, nHeight, pRGBBuf, nRGBBufSize) && IsUpOK(x, y, nWidth, nHeight, pRGBBuf, nRGBBufSize))
				{
					point->x = x + 1;
					point->y = y + 1;
					ret = 0;
					break;
				}
			}
		}
		if (ret == 0) break;
	}
	return ret;
}

//find the range of red, width>100 and height >100
int CReceiveFile::FindDataRang()
{
	//return 0;

	//uint32_t screen_width = GetSystemMetrics(SM_CXSCREEN);
	//uint32_t screen_height = GetSystemMetrics(SM_CYSCREEN);

	RECT rc;
	
	::GetClientRect(m_ctx.hWnd, &rc);
	uint32_t screen_width = rc.right;
	uint32_t screen_height = rc.bottom;

	uint32_t nRGBBufSize = screen_width * screen_height * 4;
	BYTE* pRGBBuf = new BYTE[nRGBBufSize];
	int ret = GetRGBDataFromScreenRect(0, 0, screen_width, screen_height, pRGBBuf, nRGBBufSize);
	if (ret == 0)
	{
		m_rect.SetRectEmpty();

		POINT point;
		ret = GetLeftTop(&point, screen_width, screen_height, pRGBBuf, nRGBBufSize);
		if (ret == 0) 
		{
			m_rect.left = point.x; 
			m_rect.top = point.y;
		}
	}
	if (ret == 0)
	{
		POINT point;
		ret = GetRightBottom(&point, screen_width, screen_height, pRGBBuf, nRGBBufSize);
		if (ret == 0)
		{
			m_rect.right = point.x;
			m_rect.bottom = point.y;
		}
	}
	if (ret == 0)
	{
		ret = GetRGBDataFromScreenRect(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), pRGBBuf, nRGBBufSize);
		if (ret == 0)
		{
			int nCount = m_rect.Width() * m_rect.Height();
			for (int p = 0; p < nCount; p++)
			{
				if (!((pRGBBuf[p * 3] == 0) && (pRGBBuf[p * 3 + 1] == 0) && (pRGBBuf[p * 3 + 2] == 0xFF)))
				{
					m_dlg->Log(_T("Check red RECT failed!"));
					ret = -1;
					break;
				}
			}
		}
	}

	delete[] pRGBBuf;

	return ret;
}

/**
 *@brief 定EnumWindows()的回调函数。每遍历一个窗口，调用一次。
 *@param[in] hwnd:所获取的句柄。
 *@param[in] lParam:EnumWindows()第二个参数的输入。
 *@return 返回TRUE，EnumWindows()继续遍历窗口。FALSE停止遍历。
 */
static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	TCHAR windowText[256];
	GetWindowText(hwnd, windowText, 256);
	
	if (_tcslen(windowText) > 6 && _tcsstr(windowText, _T("华为云客户端")) != NULL)
	{
		HWND* phWnd = (HWND*)lParam;
		*phWnd = hwnd;
		return FALSE;
	}
	else
		return TRUE;
}

HWND CReceiveFile::FindVmWndHandle(LPCTSTR pctszTitle) const
{
	HWND hWndCloud = NULL;
	EnumWindows(EnumWindowsCallback, (LPARAM)&hWndCloud);
	return hWndCloud;
}

void CReceiveFile::InitDC()
{
	DeinitDC();

	m_ctx.hdc = ::GetDC(m_ctx.hWnd);
	m_ctx.hdcMem = ::CreateCompatibleDC(m_ctx.hdc);
}

void CReceiveFile::DeinitDC()
{
	if (m_ctx.hdcMem) {
		::DeleteObject(m_ctx.hdcMem);
		m_ctx.hdcMem = NULL;
	}
	if (m_ctx.hdc) {
		::ReleaseDC(m_ctx.hWnd, m_ctx.hdc);
		m_ctx.hdc = NULL;
	}
}

bool CReceiveFile::Connect()
{
	bool bConn = false;

	if (FindDataRang() == 0)
	{
		if (m_pBuf) delete[] m_pBuf;
		m_nBufSize = m_rect.Width() * m_rect.Height() * 4;
		m_pBuf = new BYTE[m_nBufSize];

		Reply(REPLY_CONTINUE);
		m_dlg->Log(_T("find Rect Left top, right bottom %d %d %d %d\n"), m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
		if (IsDataReadable(CONNECTION_WAIT_TIMEOUT, -1) == 0)
		{
			bConn = true;
		}
		else
		{
			m_dlg->Log(_T("connected timeout!"));
		}
	}
	else
		m_dlg->Log(_T("Find range error!"));


	if(bConn)
		m_dlg->Log(_T("connected."));
	else
	{
		m_dlg->Log(_T("connected failed."));
	}

	return bConn;
}

void CReceiveFile::Disconnect()
{
	DeinitDC();
	
	if (m_pBuf)
	{
		delete[] m_pBuf;
		m_pBuf = NULL;
	}
}

int CReceiveFile::ReceiveFile(LPCTSTR pctszFileName)
{
	int ret = 0;

	if (Connect())
	{
		m_dlg->Log(_T("Begin save file:%s"), pctszFileName);

		file_info_t file_info;
		if (GetFileInfo(&file_info) == 0)
		{
			UINT nRetry = 1;
			uint64_t nFileChecksum = 0;
			uint32_t nLastId = 0;
			int64_t nRemain = file_info.filesize;
			m_dlg->Log(_T("Receive file:%s, File size:%lld, check sum:%lld."), file_info.filename, file_info.filesize, file_info.checksum);

			CFile file;
			CString strFileName;

			if (pctszFileName && _tcslen(pctszFileName) > 0) 
				strFileName = pctszFileName;
			else
			{
				strFileName = "c:";
				TCHAR *ret = _tcsrchr(file_info.filename, _T('\\'));
				strFileName += ret;
			}

			if (file.Open(strFileName, CFile::modeCreate | CFile::modeWrite))
			{	
				Reply(REPLY_CONTINUE);
				do
				{
					if (m_dlg->IsAbort()) break;

					ret = IsDataReadable(RW_WAIT_TIMEOUT, nLastId);
					if (ret == 0)
					{
						frame_header_t* fh = (frame_header_t*)m_pBuf;
						nFileChecksum += fh->nCheckSum;

						static ULONGLONG oldTickcount = GetTickCount64();
						TCHAR ch[100];
						_stprintf_s(ch, 100, _T("Speed:%lldKB,remain:%lldKB."), (sizeof(frame_header_t) + fh->nDataSize) / (1 + GetTickCount64() - oldTickcount), nRemain / 1024);
						oldTickcount = GetTickCount64();
						m_dlg->DisplaySpeed(ch);

						nRetry = 0;
						file.Write(m_pBuf + sizeof(frame_header_t), fh->nDataSize);
						nRemain -= fh->nDataSize;

						m_dlg->Log(_T("Receiving File data id:%d, remain:%lldKB."), fh->nId, nRemain / 1024);
						if (nRemain == 0)
						{
							m_dlg->Log(_T("ReceiveFile completed."));
							Reply(REPLY_CONTINUE);
							break;
						}
						else if (nRemain < 0)
						{
							m_dlg->Log(_T("ReceiveFile completed. but has error!"));
							ret = -1;
							Reply(REPLY_ERROR);
							break;
						}
						else
						{
							nLastId++;
							Reply(REPLY_CONTINUE);
						}
					}
					else
					{
						if (nRetry > MAX_RETRY_TIMES)
						{
							ret = -1;
							m_dlg->Log(_T("ReceiveFile reach max retry times"));
							break;
						}
						nRetry++;
						Reply(REPLY_RETRY);
					}
				} while (true);

				if (nFileChecksum != file_info.checksum)
				{
					m_dlg->Log(_T("ReceiveFile checksum failed!"));
					ret = -1;
				}

				if (nRemain > 0)
				{
					m_dlg->Log(_T("ReceiveFile remain:%lldKB, timeout!"), nRemain / 1024);
					ret = -1;
				}
				file.Close();
			}
			else
			{
				ret = -1;
				m_dlg->Log(_T("Open Receive File: %s failed."), pctszFileName);
			}
		}
		else
		{
			ret = -1;
			m_dlg->Log(_T("Receive file: get log file info error!"));
		}
		Disconnect();
	}
	else
		ret = -1;
	return ret;
}

int CReceiveFile::IsDataReadable(uint32_t timeout, int32_t nLastId)
{
	int ret = RET_TIMEOUT;
	time_t oldTime = time(NULL);

	frame_header_t *fh = (frame_header_t*)m_pBuf;
	m_dlg->Log(_T("IsDataReadable begin, fh.id:%d"), fh->nId);
	while ((time(NULL) - oldTime) < timeout)
	{
		GetRGBDataFromScreenRect();
		fh = (frame_header_t*)m_pBuf;
		if (fh->nId == nLastId + 1)
		{
			m_dlg->Log(_T("fh id:%d, DataSize:%d, CheckSum:%d"), fh->nId, fh->nDataSize, fh->nCheckSum);
			uint64_t nCalFrameCheckSum = fh->nDataSize > (m_nBufSize - sizeof(frame_header_t)) ? 0 : CalCheckSum(m_pBuf + sizeof(frame_header_t), fh->nDataSize);
			uint32_t nCalCheckSum = (uint32_t)nCalFrameCheckSum;
			if (fh->nCheckSum == nCalCheckSum)
			{
				ret = 0;
				break;
			}
			else
			{
				Sleep(0);
				m_dlg->Log(_T("checksum error. fh id:%d, DataSize:%d, CheckSum:%d"), fh->nId, fh->nDataSize, fh->nCheckSum);
				continue;
			}
		}
		else
			;// m_dlg->Log(_T("different id, lastId:%d, fh id:%d, DataSize:%d, CheckSum:%d"), nLastId, fh->nId, fh->nDataSize, fh->nCheckSum);

		Sleep(50);
	};
	m_dlg->Log(_T("IsDataReadable end, fh.id:%d"), fh->nId);

	return ret;
}

int  CReceiveFile::GetFileInfo(file_info_t* file_info) 
{
	int ret = 0;
	*file_info = *((file_info_t*)m_pBuf);
	return ret;
}

int CReceiveFile::GetRGBDataFromScreenRect() 
{
	//m_dlg->Log(_T("GetRGBDataFromScreenRect begin."));
#if 0
	GetRGBDataFromScreenRect(m_rect.left, m_rect.top, m_rect.Width()/2, m_rect.Height(), m_pBuf, m_nBufSize/2);
	GetRGBDataFromScreenRect(m_rect.left+100, m_rect.top + 100, m_rect.Width() / 2, m_rect.Height(), m_pBuf + m_nBufSize / 2, m_nBufSize/2);

	CString str = _T("GetRGBDataFromScreenRect ");
	for (size_t i = 0; i < 40*40*3; i++)
	{
		CString s;
		s.Format(_T("%02X "), m_pBuf[i]);
		str += s;
	}
	m_dlg->Log(str);
#else
	GetRGBDataFromScreenRect(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), m_pBuf, m_nBufSize);

	// below is important, in local copy must uncomment below lines
#if LOCAL_COPY
	CString str = _T("GetRGBDataFromScreenRect ");
	for (size_t i = 0; i < (size_t)20 * 20 * 3; i++)
	{
		CString s;
		s.Format(_T("%02X "), m_pBuf[i]);
		str += s;
	}	
	m_dlg->Log(str);
#endif
#endif

//	// copy screen to local
//	CRect rc;
//	rc = m_rect;
//	rc.right += 1;
//	::BitBlt(m_hdcDesktop, rc.left, rc.top, rc.Width(), rc.Height(), m_ctx.hdc, m_rect.left, m_rect.top, SRCCOPY);
//
//	//HBITMAP hBitmap = ::CreateCompatibleBitmap(m_desktop_ctx.hdc, nWidth, nHeight);
//	//HGDIOBJ hOldBmp = ::SelectObject(m_desktop_ctx.hdcMem, hBitmap);
//	////copy desktop to bitmap
//	//::BitBlt(m_desktop_ctx.hdcMem, 0, 0, nWidth, nHeight, m_desktop_ctx.hdc, nx, ny, SRCCOPY);
//
//	return GetRGBDataFromScreenRect(rc.left, rc.top, rc.Width()-1, rc.Height(), m_pBuf, m_nBufSize);

	//m_dlg->Log(_T("GetRGBDataFromScreenRect end."));
	return 	0;
}

int CReceiveFile::GetRGBDataFromScreenRect(uint32_t nx, uint32_t ny, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) 
{
	if (pRGBBuf == NULL) return -1;
	if (nRGBBufSize < nWidth * nHeight * 4) return -1;

	int ret = -1;
	
	HBITMAP hBitmap = ::CreateCompatibleBitmap(m_ctx.hdc, nWidth, nHeight);
	//get bitmap from RGB data
	if (hBitmap != NULL)
	{
		HGDIOBJ hOldBmp = ::SelectObject(m_ctx.hdcMem, hBitmap);
		// copy desktop to bitmap
		::BitBlt(m_ctx.hdcMem, 0, 0, nWidth, nHeight, m_ctx.hdc, nx, ny, SRCCOPY);
		
		CBitmap* bitmap = CBitmap::FromHandle(hBitmap);
		BITMAP bmp;
		bitmap->GetBitmap(&bmp);
		uint32_t bitmapSize = bmp.bmWidth * bmp.bmHeight * 4;
		if (bitmap->GetBitmapBits(bitmapSize, pRGBBuf))
		{
			//32->24,delete the last 1 byte；
			uint32_t pixel_count = nWidth * nHeight * 4;
			uint32_t src = 3;
			for (uint32_t nCount = 4; nCount < pixel_count;)
			{
				pRGBBuf[src++] = pRGBBuf[nCount++];
				pRGBBuf[src++] = pRGBBuf[nCount++];
				pRGBBuf[src++] = pRGBBuf[nCount++];
				nCount++;
			}
			ret = 0;
		}
		else
		{
			m_dlg->Log(_T("Failed, GetRGBDataFromScreenRect rect bitmap is nullA."));
		}
		bitmap->Detach();
		//bitmap->DeleteObject();

		::SelectObject(m_ctx.hdcMem, hOldBmp);
		::DeleteObject(hBitmap);
	}
	else
		m_dlg->Log(_T("Bitmap Failed, GetRGBDataFromScreenRect rect bitmap is null."));

	return ret;
}

void CReceiveFile::Reply(LPCTSTR pctszReply) const
{
	CopyToClipboard(pctszReply);
	m_dlg->Log(_T("reply %s to send"), pctszReply);
}
