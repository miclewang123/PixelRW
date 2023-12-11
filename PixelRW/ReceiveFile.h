#pragma once

#include "CBase.h"

// Receive File process:
// call function Connect() to find screen data transfer range. 
// the range is a rect of red color, and size is bigger than 100*100;
// note:there must be only one range comfort to this condition;
// the last line is flag line, not use to transfer data.
// ACK: fill green in the last line of the rect.
// query the color of last line in the rect, if the color changed to red, read below.
// send file size(8Bytes), checksum value(8Bytes) and file name, and make the last line green.
// 
// cycle run below action.
// query the color of last line in the rect, if the color changed to red, begin read data,
//
// until transfer data complete, return 0. 
// if timeout in wait the last line green, send failed, return -1.
class CReceiveFile : public CBase{
public:
	CReceiveFile(CPixelRWDlg* dlg, CRect& rect);
	~CReceiveFile();

public:
	bool Connect();
	int ReceiveFile(LPCTSTR pctszFileName);
	void Disconnect();

private:
	void InitDC();
	void DeinitDC();

	HWND FindVmWndHandle(LPCTSTR pctszTitle) const;

	//find the range of red, width>100 and height >100
	int FindDataRang();
	int GetLeftTop(POINT* point, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;
	int GetRightBottom(POINT* point, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;
	bool IsRightOK(int x, int y, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;
	bool IsDownOK(int x, int y, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;
	bool IsLeftOK(int x, int y, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;
	bool IsUpOK(int x, int y, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) const;

	int  GetFileInfo(file_info_t* file_info) ;

	int IsDataReadable(uint32_t timeout, int32_t nLastId) ;
	int GetRGBDataFromScreenRect() ;
	int GetRGBDataFromScreenRect(uint32_t nx, uint32_t ny, uint32_t nWidth, uint32_t nHeight, BYTE* pRGBBuf, uint32_t nRGBBufSize) ;

	void Reply(LPCTSTR pctszReply) const;

private:
	CRect	m_rect;			// rect to transfer data
	
	BYTE*	m_pBuf;			// Screen rect buf
	uint32_t m_nBufSize;	// buf size in` COLORREF format

	context_t m_ctx;
};