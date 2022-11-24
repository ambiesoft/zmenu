
// zmenuOptionDlg.cpp : implementation file
//

#include "pch.h"
#include "../../profile/cpp/Profile/include/ambiesoft.profile.h"
#include "../../lsMisc/GetVersionString.h"

#include "framework.h"
#include "zmenuOption.h"
#include "zmenuOptionDlg.h"
#include "afxdialogex.h"
#include "About.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace Ambiesoft;

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
, m_strInfo(_T(""))
{
	m_strInfo = stdFormat(L"%s v%s",
		L"zmenu",
		GetVersionString(theApp.GetzmenuExe().c_str(), 3).c_str()).c_str();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_INFO, m_strInfo);
	DDX_Control(pDX, IDC_STATIC_INFO, m_staticInfo);
	DDX_Control(pDX, IDC_STATIC_COPYRIGHT, m_staticCopyRight);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()




BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	std::vector<HWND> excepts;
	excepts.push_back(m_staticInfo);
	excepts.push_back(m_staticCopyRight);
	i18nChangeDialogText(m_hWnd, &excepts[0], (int)excepts.size());
	SetWindowText(stdFormat(I18N(L"About %s"), L"zmenu").c_str());

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
