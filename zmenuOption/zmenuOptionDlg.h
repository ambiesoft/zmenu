
// zmenuOptionDlg.h : header file
//

#pragma once


// CzmenuOptionDlg dialog
class CzmenuOptionDlg : public CDialogEx
{
// Construction
public:
	CzmenuOptionDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ZMENUOPTION_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	BOOL m_bShowHidden;
	afx_msg void OnBnClickedOk();
	BOOL m_bNoIcons;
	afx_msg void OnBnClickedButtonAbout();
	afx_msg void OnBnClickedButtonPintotaskbar();
};
