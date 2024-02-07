#pragma once

#include "PixelRWDlg.h"

#define REQUEST_CONTINUE			_T("CONTINUE")
#define REQUEST_RETRY				_T("RETRY")
#define REQUEST_ERROR				_T("ERROR")
#define REQUEST_COMPLETE			_T("COMPLETE")

#define CONNECTION_WAIT_TIMEOUT		10
#define RW_WAIT_TIMEOUT				4

#define MIN_RECT_WIDTH				10
#define MIN_RECT_HEIGHT				10

#define RET_COMPLETE				1
#define RET_NORMAL					0
#define RET_ERROR					(-1)
#define RET_TIMEOUT					(-2)

#define	SPLIT_COUNT					1

struct frame_header_t
{
	int32_t nId;
	uint32_t nDataSize;
	uint32_t nCheckSum;
};

struct file_info_t
{
	frame_header_t fh;
	
	uint64_t nFileSize;
	uint64_t nFileCheckSum;
	uint64_t nLastPos;
	TCHAR	 tchFileName[MAX_PATH];
};

struct context_t
{
	HWND hWnd;
	HDC hdc;
	HDC hdcMem;
};

class CBase
{
public:
	CBase(CPixelRWDlg* dlg) { m_dlg = dlg; }
	virtual ~CBase(){}

public:
	bool CopyToClipboard(LPCTSTR str) const;
	bool EmptyClipboard() const;
	bool GetTextFromClipboard(LPTSTR ptszBuf, size_t nBufSize) const;
	uint64_t CalCheckSum(const BYTE* buf, size_t nBufSize) const;

	uint64_t GetFileCheckSum(CFile* file) const;

protected:
	CPixelRWDlg* m_dlg;
};

// problem:
// 1. remote copy clipboard hWnd must be NULL;
// 2. remote 大图像无法传输；
// 3. split has problem;

// 3. local copy must add log info to avoid error.  it is OK now.
// 4. before use must first run copy/paste manually. it is OK now.
