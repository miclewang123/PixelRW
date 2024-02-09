#include "pch.h"
#include "CBase.h"

bool CBase::CopyToClipboard(LPCTSTR str) const
{
	bool ret = false;

	//if (str && _tcslen(str) > 0 && ::OpenClipboard(AfxGetMainWnd()->GetSafeHwnd()))
	if (str && _tcslen(str) > 0 && ::OpenClipboard(NULL) /* && ::EmptyClipboard() */)
	{
		uint32_t nStrSize = (uint32_t)(_tcslen(str) + 1) * sizeof(TCHAR);
		HGLOBAL hBuf = ::GlobalAlloc(GMEM_MOVEABLE, nStrSize);
		if (hBuf)
		{
			LPTSTR str1 = (LPTSTR)::GlobalLock(hBuf);
			if (str1) {
				memcpy(str1, str, nStrSize);
				::GlobalUnlock(hBuf);
				HANDLE h = ::SetClipboardData(CF_UNICODETEXT, hBuf);
				if(!h) m_dlg->Log(_T("CopyToClipboard error."));
				ret = true;
			}
			::GlobalFree(hBuf);
		}
		::CloseClipboard();
	}
	return ret;
}

bool CBase::EmptyClipboard() const
{
	bool ret = false;

	//if (::OpenClipboard(AfxGetMainWnd()->GetSafeHwnd()))
	if (::OpenClipboard(NULL))
	{
		::EmptyClipboard();
		::CloseClipboard();

		ret = true;
		m_dlg->Log(_T("EmptyClipboard"));
	}
	return ret;
}

bool CBase::GetTextFromClipboard(LPTSTR ptszBuf, size_t nBufSize) const
{
	bool ret = false;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		//if (::OpenClipboard(AfxGetMainWnd()->GetSafeHwnd()))
		if (::OpenClipboard(NULL))
		{
			HGLOBAL hBuf = ::GetClipboardData(CF_UNICODETEXT);
			if (NULL != hBuf)
			{
				LPTSTR str = (LPTSTR)::GlobalLock(hBuf);
				if (str && _tcslen(str) > 0)
				{
					_tcscpy_s(ptszBuf, nBufSize, str);
					::GlobalUnlock(hBuf);
					ret = true;
				}
			}
			::CloseClipboard();
		}
	}
	return ret;
}

uint64_t CBase::CalCheckSum(const BYTE* buf, size_t nBufSize) const
{
	uint64_t ret = 0;
	for (size_t i = 0; i < nBufSize; i++)
	{
		ret += buf[i];
	}
	return ret;
}

uint64_t CBase::GetFileCheckSum(CFile* file) const
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