#pragma once

#include "CBase.h"

// Send File process:
// select screen rect range to transfer data.( min width and height must >= 100 to enhance performation)
// the last line is flag line, not use to transfer data.
// fill red color in the rect
// query the color of last line in the rect, if the color changed to green, handshake is OK.
// send file size(8Bytes), checksum value(8Bytes) and file name, and make the last line red.
// 
// cycle run below action.
// query the color of last line in the rect, if the color changed to green, begin write data,
//
// until read data complete, return 0. 
// if timeout in wait the last line red, receive failed, return -1.

class CSendFile : public CBase {
public:
	CSendFile(CPixelRWDlg* dlg, CRect& rect);
	~CSendFile();

public:
	int SendFile(LPCTSTR pctszFileName);

private:
	void InitDC();
	void DeinitDC();

	bool SetDataToScreenBuf(BYTE* pData, uint32_t nDataSize) const;
	void WriteDataToScreen() ;

	uint64_t GetFileCheckSum(CFile* file) const;

	int IsDataWritable(uint32_t timeout, int nId) ;
	int FillRectRed() ;

	bool IsContinue(LPCTSTR pctszText, int nId) const;
	bool IsError(LPCTSTR pctszText) const;
	bool IsRetry(LPCTSTR pctszText) const;

private:
	CRect m_rect;			// rect to transfer data

	BYTE* m_pBuf;			// Screen rect buf
	uint32_t m_nBufSize;	// buf size in COLORREF format

	context_t m_desktop_ctx;
};
