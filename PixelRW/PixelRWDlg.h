
// PixelRWDlg.h: 头文件
//

#pragma once


// CPixelRWDlg 对话框
class CPixelRWDlg : public CDialogEx
{
// 构造
public:
	CPixelRWDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PIXELRW_DIALOG };
#endif
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;
	CListBox* m_list;
	UINT m_nCharWidth;
	BOOL m_bAbort;
	BOOL m_bRunning;
	//CStdioFile m_log_file;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	BOOL IsAbort();
	
	void Log(LPCTSTR str, ...);
	void SetReceiveFile(LPCTSTR strFileName);
	void DisplaySpeed(LPCTSTR str);

	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnReceive();
	afx_msg void OnBnClickedBtnGet();
	afx_msg void OnBnClickedBtnSet();
	afx_msg void OnBnClickedBtnTest();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedBtnSendCopy();
	afx_msg void OnBnClickedBtnReceiveCopy();
};
