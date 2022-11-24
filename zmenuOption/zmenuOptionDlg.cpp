
// zmenuOptionDlg.cpp : implementation file
//

#include "pch.h"


#include "framework.h"


#include "zmenuOption.h"
#include "zmenuOptionDlg.h"
#include "About.h"
#include "afxdialogex.h"

#include "../common/common.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace Ambiesoft::stdosd;

CzmenuOptionDlg::CzmenuOptionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ZMENUOPTION_DIALOG, pParent)
	, m_bShowHidden(FALSE)
	, m_bNoIcons(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	Profile::CHashIni ini(Profile::ReadAll(GetIniPath()));
	Profile::GetInt(SECTION_OPTION, KEY_SHOW_HIDDEN, FALSE, m_bShowHidden, ini);
	Profile::GetInt(SECTION_OPTION, KEY_NO_ICON, FALSE, m_bNoIcons, ini);
}

void CzmenuOptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_SHOWHIDDEN, m_bShowHidden);
	DDX_Check(pDX, IDC_CHECK_NOICON, m_bNoIcons);
}

BEGIN_MESSAGE_MAP(CzmenuOptionDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, &CzmenuOptionDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_ABOUT, &CzmenuOptionDlg::OnBnClickedButtonAbout)
	ON_BN_CLICKED(IDC_BUTTON_PINTOTASKBAR, &CzmenuOptionDlg::OnBnClickedButtonPintotaskbar)
END_MESSAGE_MAP()


// CzmenuOptionDlg message handlers

BOOL CzmenuOptionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	theApp.SetSingleHWND(m_hWnd);

	i18nChangeDialogText(m_hWnd);

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetWindowText(L"Option - zmenu");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CzmenuOptionDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CzmenuOptionDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CzmenuOptionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CzmenuOptionDlg::OnDestroy()
{

	theApp.ReleaseSingleHWND();

	CDialogEx::OnDestroy();
}


void CzmenuOptionDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();

	Profile::CHashIni ini(Profile::ReadAll(GetIniPath()));
	bool ok = true;
	ok &= Profile::WriteInt(SECTION_OPTION, KEY_SHOW_HIDDEN, m_bShowHidden, ini);
	ok &= Profile::WriteInt(SECTION_OPTION, KEY_NO_ICON, m_bNoIcons, ini);

	ok &= Profile::WriteAll(ini, GetIniPath());
	if (!ok)
	{
		AfxMessageBox(I18N(L"Failed to save ini."));
	}
}


void CzmenuOptionDlg::OnBnClickedButtonAbout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}


void CzmenuOptionDlg::OnBnClickedButtonPintotaskbar()
{
	ShowWindow(SW_HIDE);
	CKernelHandle handle;
	OpenCommon(m_hWnd,
		theApp.GetzmenuExe().c_str(),
		L"--pin-me",
		NULL,
		&handle);
	WaitForSingleObject(handle, INFINITE);
	ShowWindow(SW_SHOW);
}
