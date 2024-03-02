
// PixelRWDlg.cpp: 实现文件
#include "pch.h"
#include "framework.h"
#include "PixelRW.h"
#include "PixelRWDlg.h"
#include "afxdialogex.h"
#include "resource.h"

#include "SendFile.h"
#include "ReceiveFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPixelRWDlg 对话框
CPixelRWDlg::CPixelRWDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PIXELRW_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_list = NULL;
#ifdef USE_FILELOG
	if(!m_log_file.Open(_T("log.txt"), CFile::modeCreate | CFile::modeWrite | CFile::typeText))
		m_log_file.Open(_T("log1.txt"), CFile::modeCreate | CFile::modeWrite | CFile::typeText);
#endif
	m_bAbort = FALSE;
	m_bRunning = FALSE;
	m_nCharWidth = 0;
}

void CPixelRWDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPixelRWDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CPixelRWDlg::OnBnClickedBtnSend)
	ON_BN_CLICKED(IDC_BTN_RECEIVE, &CPixelRWDlg::OnBnClickedBtnReceive)
	ON_BN_CLICKED(ID_BTN_GET, &CPixelRWDlg::OnBnClickedBtnGet)
	ON_BN_CLICKED(ID_BTN_SET, &CPixelRWDlg::OnBnClickedBtnSet)
	ON_BN_CLICKED(ID_BTN_TEST, &CPixelRWDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDCANCEL, &CPixelRWDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BTN_SEND_COPY, &CPixelRWDlg::OnBnClickedBtnCopy)
	ON_BN_CLICKED(IDC_BTN_RECEIVE_COPY, &CPixelRWDlg::OnBnClickedBtnPaste)
END_MESSAGE_MAP()

// CPixelRWDlg 消息处理程序
BOOL CPixelRWDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	SetDlgItemInt(IDC_EDIT_X, 100, FALSE);
	SetDlgItemInt(IDC_EDIT_Y, 100, FALSE);
	SetDlgItemInt(IDC_EDIT_WIDTH, 32, FALSE);
	SetDlgItemInt(IDC_EDIT_HEIGHT, 22, FALSE);

	SetDlgItemText(IDC_EDIT_PREFIX, _T("A"));
	SetDlgItemText(IDC_EDIT_FILE_SEND, _T("d:\\aaa.zip"));
	SetDlgItemText(IDC_EDIT_FILE_RECEIVE, _T(""));

	m_list = (CListBox*)GetDlgItem(IDC_LIST_LOG);

	CDC* pDC = m_list->GetDC();
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	m_nCharWidth = tm.tmMaxCharWidth;
	m_list->ReleaseDC(pDC);
	
	m_bAbort = FALSE;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPixelRWDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPixelRWDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPixelRWDlg::OnBnClickedBtnSend()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bAbort = FALSE;
	m_bRunning = TRUE;

	//((CButton*)GetDlgItem(IDC_CHECK_SEND))->SetCheck(TRUE);
	GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
	m_list->ResetContent();

	CRect rc;
	rc.left = GetDlgItemInt(IDC_EDIT_X);
	rc.top = GetDlgItemInt(IDC_EDIT_Y);
	rc.right = rc.left + GetDlgItemInt(IDC_EDIT_WIDTH);
	rc.bottom = rc.top + GetDlgItemInt(IDC_EDIT_HEIGHT);
	
	CString strPrefix;
	this->GetDlgItemText(IDC_EDIT_PREFIX, strPrefix);
	CSendFile sendFile(this, rc, strPrefix);
	CString strFile;
	GetDlgItemText(IDC_EDIT_FILE_SEND, strFile);
	int ret = sendFile.SendFile(strFile);

	if (ret >= 0) Log(_T("send succeed"));
	else Log(_T("send failed"));

	GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
	m_bRunning = FALSE;
}

void CPixelRWDlg::OnBnClickedBtnReceive()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bAbort = FALSE;
	m_bRunning = TRUE;
	GetDlgItem(IDC_BTN_RECEIVE)->EnableWindow(FALSE);
	m_list->ResetContent();

	CString strPrefix;
	this->GetDlgItemText(IDC_EDIT_PREFIX, strPrefix);
	CReceiveFile recvFile(this, strPrefix);

	CString strFileName;
	GetDlgItemText(IDC_EDIT_FILE_RECEIVE, strFileName);
	int ret = recvFile.ReceiveFile(strFileName);

	if (ret == 0)Log(_T("receive succeed"));
	else Log(_T("receive failed"));

	GetDlgItem(IDC_BTN_RECEIVE)->EnableWindow(TRUE);
	m_bRunning = FALSE;
}

#define CLIPBOARD_BUFFER_SIZE		1024 * 1024 * 4

void CPixelRWDlg::OnBnClickedBtnCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strFileName = _T("D:\\copy.txt");
	SetDlgItemText(IDC_EDIT_FILE_SEND, strFileName);
	
	CFile file;
	if (file.Open(strFileName, CFile::modeCreate | CFile::modeWrite))
	{
		CBase base(this);
		TCHAR* buf = new TCHAR[CLIPBOARD_BUFFER_SIZE];
		
		base.GetTextFromClipboard(buf, CLIPBOARD_BUFFER_SIZE);

		BYTE bt[] = {0xFF, 0xFE};
		file.Write(bt, 2);
		file.Write(buf, (UINT)((_tcslen(buf) + 1) * sizeof(TCHAR)));
		delete[] buf;
		file.Close();

		OnBnClickedBtnSend();
	}
}

void CPixelRWDlg::OnBnClickedBtnPaste()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strFileName = _T("D:\\copy.txt");
	SetDlgItemText(IDC_EDIT_FILE_RECEIVE, strFileName);

	OnBnClickedBtnReceive();

	CFile file;
	if (file.Open(strFileName, CFile::modeRead ))
	{
		TCHAR* buf = new TCHAR[CLIPBOARD_BUFFER_SIZE];
		file.Read(buf, CLIPBOARD_BUFFER_SIZE);
		file.Close();

		CBase base(this);
		base.CopyToClipboard(buf);
		delete[] buf;
	}
}

BOOL CPixelRWDlg::IsAbort()
{
	return m_bAbort;
}

#define LOG_SIZE 40*40*4*3+1000

void CPixelRWDlg::Log(LPCTSTR strFormat, ...)
{
	TCHAR *buf=new TCHAR[LOG_SIZE];
	va_list args;
	va_start(args, strFormat);
	_vstprintf(buf, LOG_SIZE, strFormat, args);
	va_end(args);

	time_t t = time(NULL);
	CString str;
	str.Format(_T("%lld  %s\n"), t, buf);
	
	static ULONGLONG oldTickcount = GetTickCount64();
	static CString oldStr;
	if (oldStr != str)
	{
		m_list->InsertString(0, str);
#ifdef USE_FILELOG
		if((HANDLE)m_log_file) m_log_file.WriteString(str);
#endif
		static int nMaxStrLen = 0;
		if (nMaxStrLen < str.GetLength())
		{
			nMaxStrLen = str.GetLength();
			m_list->SetHorizontalExtent(nMaxStrLen * m_nCharWidth);
		}

		ULONGLONG nTimeDiff = GetTickCount64() - oldTickcount;
		if (nTimeDiff > 1000)
		{
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			oldTickcount = GetTickCount64();
		}

		oldStr = str;
	}

	delete[] buf;
}

void CPixelRWDlg::SetReceiveFile(LPCTSTR strFileName)
{
	SetDlgItemText(IDC_EDIT_FILE_RECEIVE, strFileName);
}

void CPixelRWDlg::DisplaySpeed(LPCTSTR str)
{
	SetDlgItemText(IDC_STATIC_SPEED, str);
	
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void CPixelRWDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bAbort = TRUE;
	if (m_bRunning) return;
#ifdef USE_FILELOG
	m_log_file.Close();
#endif
	CDialogEx::OnCancel();
}

void CPixelRWDlg::SetReceiveRange(CRect& rect)
{
	SetDlgItemInt(IDC_EDIT_X, rect.left, FALSE);
	SetDlgItemInt(IDC_EDIT_Y, rect.top, FALSE);
	SetDlgItemInt(IDC_EDIT_WIDTH, rect.Width(), FALSE);
	SetDlgItemInt(IDC_EDIT_HEIGHT, rect.Height(), FALSE);
}

////////////////////////////////////////////// test //////////////////////////////////////////
static int GetScreenRectRGB(uint32_t nx, uint32_t ny, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize)
{
	if (pRGBBuf == NULL) return -1;
	if (nRGBBufSize < nWidth * nHeight * 4) return -1;

	//获取bitmap
	//HWND hWnd = ::GetDesktopWindow();
	HWND hWnd = NULL;
	HDC hdcSrc = ::GetDC(hWnd);
	HDC hdcMem = ::CreateCompatibleDC(hdcSrc);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hdcSrc, nWidth, nHeight);
	HGDIOBJ hOldBmp = ::SelectObject(hdcMem, hBitmap);

	// copy desktop to bitmap
	::BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcSrc, nx, ny, SRCCOPY);
	
	//从bitmap提取RGB数据
	if (hBitmap != NULL)
	{
		CBitmap* bitmap = CBitmap::FromHandle(hBitmap);
		BITMAP bmp;
		bitmap->GetBitmap(&bmp);
		uint32_t bitmapSize = bmp.bmWidthBytes * bmp.bmHeight;

		bitmap->GetBitmapBits(bitmapSize, pRGBBuf);
		bitmap->Detach();
		//bitmap->DeleteObject();

		//32位转24位, 减去最后1个占位符；
		uint32_t pixel_count = nWidth * nHeight * 4;
		uint32_t src = 3;
		for (uint32_t nCount = 4; nCount < pixel_count;)
		{
			memmove(&pRGBBuf[src], &pRGBBuf[nCount], 3);
			src += 3;
			nCount += 4;
		}
	}

	::SelectObject(hdcMem, hOldBmp);
	//::DeleteObject(hBitmap);
	::DeleteObject(hdcMem);
	::ReleaseDC(hWnd, hdcSrc);
	
	return nWidth * nHeight * 3;
}

static void WriteDataToWnd(HWND hWnd, const CRect& rect, const BYTE* pbtBuf, uint32_t nBufSize, BOOL bEraseBackground=TRUE)
{
	HDC hdcWnd = ::GetDC(hWnd);
	HDC hdcMem = ::CreateCompatibleDC(hdcWnd);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hdcWnd, rect.Width(), rect.Height());

	// make 3bytes to 1 pixel(4bytes)
	uint32_t nBufSize1 = rect.Width() * rect.Height() * 4;
	BYTE* pbtBuf1 = new BYTE[nBufSize1];
	ZeroMemory(pbtBuf1, nBufSize1);
	uint32_t dst = 0;
	for (uint32_t src = 0; src < nBufSize;)
	{
		memcpy(pbtBuf1 + dst, pbtBuf + src, 3);
		src += 3;
		dst += 4;
	}
	
	LONG ret = ::SetBitmapBits(hBitmap, nBufSize1, pbtBuf1);
	delete[] pbtBuf1;
	if (ret)
	{
		if (bEraseBackground)
		{
			CRect rect1 = rect;
			rect1.right += 20;
			rect1.bottom += 20;
			FillRect(hdcWnd, &rect1, CBrush(RGB(255, 255, 255)));
		}
		
		HGDIOBJ hOldBmp = ::SelectObject(hdcMem, hBitmap);
		::BitBlt(hdcWnd, rect.left, rect.top, rect.Width(), rect.Height(), hdcMem, 0, 0, SRCCOPY);

		::SelectObject(hdcMem, hOldBmp);
		::DeleteObject(hBitmap);
	}

	::DeleteObject(hdcMem);
	::ReleaseDC(hWnd, hdcWnd);
}

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	TCHAR windowText[256];
	GetWindowText(hwnd, windowText, 256);

	if (_tcslen(windowText) > 6 && _tcsstr(windowText, _T("华为云客户端")) != NULL)
		//if (_tcslen(windowText) > 6 && _tcsstr(windowText, _T("Microsoft Spy")) != NULL)
	{
		HWND* phWnd = (HWND*)lParam;
		*phWnd = hwnd;
		return FALSE;
	}
	else
		return TRUE;
}

void CPixelRWDlg::OnBnClickedBtnGet()
{
	// TODO: 在此添加控件通知处理程序代码
	CRect rc;
	rc.left = GetDlgItemInt(IDC_EDIT_X);
	rc.top = GetDlgItemInt(IDC_EDIT_Y);
	rc.right = rc.left + GetDlgItemInt(IDC_EDIT_WIDTH);
	rc.bottom = rc.top + GetDlgItemInt(IDC_EDIT_HEIGHT);

	uint32_t nBufSize = rc.Width() * rc.Height() * 4;
	BYTE *pbtBuf = new BYTE[nBufSize];
	
	// 1
	nBufSize = GetScreenRectRGB(rc.left, rc.top, rc.Width(), rc.Height(), pbtBuf, nBufSize);
	CString str;
	TCHAR tch[10];
	for (uint32_t i = 0; i <= nBufSize; i++)
	{
		_stprintf_s(tch, 10, _T("%02X "), pbtBuf[i]);
		str += tch;
	}
	Log(str);

	for (uint32_t i = 0; i < nBufSize; i++)
	{
		if (i > 0 && (BYTE(pbtBuf[i] - pbtBuf[i - 1])) != 1)
			Log(_T("Bitmap error, pos is %d."), i);
	}

	CRect rect;
	CWnd *pWndLog = GetDlgItem(IDC_LIST_LOG);
	pWndLog->GetClientRect(&rect);
	rect.top = 440;
	rect.bottom = rect.top + rc.Height();
	rect.left = 10;
	rect.right = rect.left + rc.Width();
	WriteDataToWnd(this->GetSafeHwnd(), rect, pbtBuf, nBufSize);

	// 2
	/*rc.left += 100;
	rc.right += 100;
	nBufSize = rc.Width() * rc.Height() * 4;
	nBufSize = GetScreenRectRGB(rc.left, rc.top, rc.Width(), rc.Height(), pbtBuf, nBufSize);
	str = "";
	for (uint32_t i = 0; i <= nBufSize; i++)
	{
		_stprintf_s(tch, 10, _T("%02X "), pbtBuf[i]);
		str += tch;
	}
	Log(str);

	for (uint32_t i = 0; i < nBufSize; i++)
	{
		if (i > 0 && (BYTE(pbtBuf[i] - pbtBuf[i - 1])) != 1)
			Log(_T("Bitmap error, pos is %d."), i);
	}
	
	rect.left = 100;
	rect.right = rect.left + rc.Width();
	WriteDataToWnd(this->GetSafeHwnd(), rect, pbtBuf, nBufSize);*/

	delete[] pbtBuf;
}

void CPixelRWDlg::OnBnClickedBtnSet()
{
	// TODO: 在此添加控件通知处理程序代码
	CRect rc;
	rc.left = GetDlgItemInt(IDC_EDIT_X);
	rc.top = GetDlgItemInt(IDC_EDIT_Y);
	rc.right = rc.left + GetDlgItemInt(IDC_EDIT_WIDTH);
	rc.bottom = rc.top + GetDlgItemInt(IDC_EDIT_HEIGHT);

	static byte bt = 0;
	uint32_t nBufSize = rc.Width() * rc.Height() * 3;
	BYTE* pbtBuf = new BYTE[nBufSize];
	
	// 1
	for (uint32_t i = 0; i < nBufSize; i++)
	{
		pbtBuf[i] = bt++;
	}
	//WriteDataToWnd(::GetDesktopWindow(), rc, pbtBuf, nBufSize, FALSE);
	WriteDataToWnd(GetSafeHwnd(), rc, pbtBuf, nBufSize, FALSE);

	// 2
	/*for (uint32_t i = 0; i < nBufSize; i++)
	{
		pbtBuf[i] = bt++;
	}
	rc.left += 100;
	rc.right += 100;
	WriteDataToWnd(::GetDesktopWindow(), rc, pbtBuf, nBufSize, FALSE);
	*/
	delete[] pbtBuf;
}

void CPixelRWDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bRunning = TRUE;

	CDC *dcSrc = GetDesktopWindow()->GetWindowDC();
	CDC *dcDst = GetWindowDC();

	CRect rc;
	rc.left = GetDlgItemInt(IDC_EDIT_X);
	rc.top = GetDlgItemInt(IDC_EDIT_Y);
	rc.right = rc.left + 600;//GetDlgItemInt(IDC_EDIT_WIDTH);
	rc.bottom = rc.top + 300;// GetDlgItemInt(IDC_EDIT_HEIGHT);

	CRect rc1;
	rc1.left = 10;
	rc1.top = 420;
	rc1.right = 600;
	rc1.bottom = 740;

	dcDst->FillSolidRect(&rc1, RGB(0,0,0));
	dcDst->BitBlt(rc1.left, rc1.top, rc1.Width(), rc1.Height(), dcSrc, rc.left, rc.top, SRCCOPY);

	dcSrc->DeleteDC();
	dcDst->DeleteDC();

	CDC* dc_screen = GetDesktopWindow()->GetWindowDC();
	COLORREF c;
	CString str;
	int x = GetDlgItemInt(IDC_EDIT_X2);
	int y = GetDlgItemInt(IDC_EDIT_Y2);
	for (size_t i = 0; i <50; i++)
	{
		if (m_bAbort) break;

		c =  dc_screen->SetPixel(x, y, RGB(0xff, 0, 0));
		str.Format(_T("%06X"), c);
		SetDlgItemText(IDC_EDIT_COLOR, str);

		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		c = dc_screen->SetPixel(x, y, RGB(0xff, 0xff, 0xff));
		str.Format(_T("%06X"), c);
		SetDlgItemText(IDC_EDIT_COLOR, str);
		
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(10);
	}

	dc_screen->DeleteDC();

	m_bRunning = FALSE;
}
