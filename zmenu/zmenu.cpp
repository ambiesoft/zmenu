#include <Windows.h>
#include <shlobj_core.h>
#include <Shlwapi.h>

#include <string>
#include <map>
#include <memory>
#include <deque>
#include <list>

#include "../../lsMisc/CreateSimpleWindow.h"
#include "../../lsMisc/GetLastErrorString.h"
#include "../../lsMisc/GetFilesInfo.h"
#include "../../lsMisc/OpenCommon.h"
#include "../../lsMisc/DebugMacro.h"
#include "../../lsMisc/CommandLineParser.h"
#include "../../lsMisc/GetVersionString.h"
#include "../../lsMisc/HighDPI.h"
#include "../../lsMisc/CHandle.h"
#include "../../lsMisc/stop_watch.h"
#include "../../lsMisc/Is64.h"
#include "../../lsMisc/CreateShortcutFile.h"
#include "../../lsMisc/stdosd/stdosd.h"
#include "../../lsMisc/I18N.h"
#include "../../lsMisc/BringWinTop.h"
#include "../../lsMisc/GetFileNameFromHwnd.h"
#include "../../profile/cpp/Profile/include/ambiesoft.profile.h"

#include "common.h"

#include "gitrev.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Msimg32.lib")

using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;
using namespace std;

#define APPNAME L"ShowQL"
#define WAIT_AFTER_LAUNCH (5 * 1000)
#define WAIT_FOR_PROCESSIDLE (30 * 1000)

CHMenu ghPopup;
TCHAR szT[MAX_PATH];
CHFont gMenuFont;

enum {
	MENUID_DUMMY = 1,
	MENUID_NOITEM,
	MENUID_OPTIONS,
	MENUID_START,
	MENUID_END = 65535 / 2,
};
HINSTANCE ghInst;
WORD gMenuIndex;
map<UINT, HWND> gCmdMap;
map<HMENU, wstring> gPopupMap;

bool gbNoIcon = false;
bool gbShowHidden = false;
int gIconWidth, gIconHeight;
UINT gItemHeight;
UINT gItemDeltaY;
UINT gItemDeltaX;


#ifndef NDEBUG

#define DEBUG_DRAW
//#define DEBUG_INITPOPUP

unique_ptr<wstop_watch> gsw;
#define TRACE_STOPWATCH(S) do { OutputDebugString(S);OutputDebugString(L" "); OutputDebugString(gsw->lookDiff().c_str()); OutputDebugString(L"\r\n"); }while(false)

#ifdef DEBUG_DRAW
#define DTRACE_DRAW(s) DTRACE(s)
#else
#define DTRACE_DRAW(s) do{}while(false)
#endif

#ifdef DEBUG_INITPOPUP
#define DTRACE_INITPOPUP(s) DTRACE(s)
#else
#define DTRACE_INITPOPUP(s) do{}while(false)
#endif

#else
#define TRACE_STOPWATCH(S) do{}while(false)
#define DTRACE_DRAW(s) do{}while(false)
#define DTRACE_INITPOPUP(s) do{}while(false)
#endif

void ErrorExit(const wchar_t* pMessage, int ret = -1)
{
	MessageBox(nullptr,
		pMessage,
		APPNAME,
		MB_ICONERROR);
	ExitProcess(ret);
}
void ErrorExit(const wstring& message)
{
	ErrorExit(message.c_str());
}
void ErrorExit(DWORD le)
{
	ErrorExit(GetLastErrorString(le).c_str(), le);
}

CHIcon getIconFromPath_obsolete(HWND hWnd)
{
	wstring targetExe = GetFileNameFromHwndAsWstring(hWnd);
	if (targetExe.empty())
		return CHIcon();
	wstring iconPath;

	SHFILEINFO sfi = { 0 };
	if (!SHGetFileInfo(targetExe.c_str(),
		0,
		&sfi,
		sizeof(sfi),
		SHGFI_ICON | SHGFI_SMALLICON))
	{
		ErrorExit(GetLastError());
	}
	return CHIcon(sfi.hIcon);
}

CHIcon getIconFromHwnd(HWND hWnd)
{
	CHIcon a ((HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0));
	return a;
}
void makeOwnerDraw(HMENU hMenu, UINT cmd)
{
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	if (!GetMenuItemInfo(hMenu, cmd, FALSE, &mii))
		ErrorExit(GetLastError());
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.fType |= MFT_OWNERDRAW;
	if (!SetMenuItemInfo(hMenu, cmd, FALSE, &mii))
		ErrorExit(GetLastError());
}
void setMenuHidden(HMENU hMenu, UINT_PTR cmd)
{
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_DATA;
	mii.dwItemData = 1;
	if (!SetMenuItemInfo(hMenu, (UINT)cmd, FALSE, &mii))
		ErrorExit(GetLastError());
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_MENUSELECT:
		//{
		//	WORD index = LOWORD(wParam);
		//	HMENU hMenu = (HMENU)lParam;
		//	GetMenuString(hMenu, index, szT, _countof(szT), MF_BYPOSITION);
		//	DTRACE(wstring() + L"WM_MENUSELECT : " + szT);
		//	glastSelectedPopup = szT;
		//}
		//break;

	case WM_MEASUREITEM:
	{
		MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
		GetMenuString(ghPopup, mis->itemID, szT, _countof(szT), MF_BYCOMMAND);
		CHDC dc(GetDC(hWnd));
		HFONT old = (HFONT)SelectObject(dc, gMenuFont);
		SIZE size;
		if (!GetTextExtentPoint32(dc, szT, lstrlen(szT), &size))
			ErrorExit(GetLastError());
		mis->itemHeight = gItemHeight;
		mis->itemWidth = gIconWidth + gItemDeltaX + size.cx;
		SelectObject(dc, old);
		return TRUE;
	}
	break;

	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

		//MENUITEMINFO mii = { 0 };
		//mii.cbSize = sizeof(mii);
		//mii.fMask = MIIM_DATA;
		//if (!GetMenuItemInfo(ghPopup, dis->itemID, FALSE, &mii))
		//	ErrorExit(GetLastError());
		const bool bHidden = dis->itemData ? true : false;

		DTRACE_DRAW(stdFormat(L"itemAction=%d, itemState=%d, hidden=%s",
			dis->itemAction,
			dis->itemState,
			bHidden ? L"Hidden" : L"Normal"
		));

		// dis->itemAction
		// ODA_DRAWENTIRE = 0x0001
		//   The entire control needs to be drawn.
		// ODA_FOCUS = 0x0004
		//   The control has lost or gained the keyboard focus.The itemState member should be 
		//   checked to determine whether the control has the focus.
		// ODA_SELECT = 0x0002
		//   The selection status has changed.The itemState member should be checked to determine 
		//   the new selection state.

		int colorMenuText = -1;
		int colorMuenuTextBk = -1;
		int colorBk = -1;
		if (dis->itemAction == ODA_DRAWENTIRE)
		{
			colorMenuText = bHidden ? COLOR_GRAYTEXT : COLOR_MENUTEXT;
		}
		else if (dis->itemAction == ODA_SELECT)
		{
			if (dis->itemState & ODS_SELECTED)
			{
				// newly selected
				colorMuenuTextBk = COLOR_MENUHILIGHT;
				colorMenuText = COLOR_HIGHLIGHTTEXT;
				colorBk = COLOR_MENUHILIGHT;
			}
			else
			{
				// deselected
				colorMenuText = bHidden ? COLOR_GRAYTEXT : COLOR_MENUTEXT;
				colorBk = COLOR_MENU;
			}
		}
		else
			assert(false);

		assert(colorMenuText != -1);
		SetTextColor(dis->hDC, GetSysColor(colorMenuText));
		if (colorMuenuTextBk != -1)
			SetBkColor(dis->hDC, GetSysColor(colorMuenuTextBk));
		if (colorBk != -1)
		{
			HBRUSH brush = GetSysColorBrush(colorBk);
			HBRUSH old = (HBRUSH)SelectObject(dis->hDC, brush);
			FillRect(dis->hDC, &dis->rcItem, brush);
			SelectObject(dis->hDC, old);
		}

		CHIcon hhh(getIconFromHwnd(gCmdMap[dis->itemID]));
		if (!DrawIconEx(dis->hDC,
			dis->rcItem.left, dis->rcItem.top + gItemDeltaY,
			hhh,
			gIconWidth, gIconHeight,
			0, 0, DI_MASK | DI_IMAGE))
		{
			ErrorExit(GetLastError());
		}

		RECT rS = dis->rcItem;
		rS.top += gItemDeltaY;
		rS.left += gIconWidth + gItemDeltaX;
		GetMenuString(ghPopup, dis->itemID, szT, _countof(szT), MF_BYCOMMAND);
		DTRACE_DRAW(stdFormat(L"DrawText=%s", szT));
		DrawText(dis->hDC, szT, lstrlen(szT), &rS, 0);

		return TRUE;
	}
	break;
	case WM_INITMENUPOPUP:
	{
		HMENU hMenu = (HMENU)wParam;
		WORD index = LOWORD(lParam);
		TRACE_STOPWATCH(L"WM_INITMENUPOPUP");
		while (DeleteMenu(hMenu, 0, MF_BYPOSITION))
			;

		const wstring sel = gPopupMap[hMenu];
		const bool bTopPopup = sel.empty();
		DTRACE_INITPOPUP(L"sel=" + sel);

		TRACE_STOPWATCH(L"WM_INITMENUPOPUP");

		for (HWND h = GetTopWindow(nullptr); h != nullptr; h = GetNextWindow(h, GW_HWNDNEXT))
		{
			if (!IsWindowVisible(h))
				continue;

			DTRACE_INITPOPUP(stdFormat(L"WM_INITMENUPOPUP:HWND:") + to_wstring(h));
			TRACE_STOPWATCH(L"WM_INITMENUPOPUP process file");
			UINT cmd = gMenuIndex++ + MENUID_START;

			
			AppendMenu(hMenu,
				MF_BYCOMMAND,
				cmd,
				GetWindowTextString(h).c_str());
			if (!gbNoIcon)
			{
				makeOwnerDraw(hMenu, cmd);
			}

			gCmdMap[cmd] = h;
		}
		if (0 == GetMenuItemCount(hMenu))
		{
			InsertMenu(hMenu, 0, MF_BYCOMMAND | MF_DISABLED, MENUID_NOITEM, L"<No Items>");
		}
		if (bTopPopup)
		{
			AppendMenu(hMenu, 0, MF_SEPARATOR, 0);
			InsertMenu(hMenu, GetMenuItemCount(hMenu), MF_BYPOSITION | MF_BYCOMMAND, MENUID_OPTIONS, I18N(L"&Options..."));

			if (true) // !gbNoRecentItems)
			{
				//// Insert recent at top
				//HMENU hPopupRecent = CreatePopupMenu();
				//InsertMenu(hMenu, 0, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hPopupRecent, I18N(L"Recent Items"));
				//InsertMenu(hMenu, 1, MF_SEPARATOR | MF_BYPOSITION, 0, 0);
				//UINT cmd = gMenuIndex++ + MENUID_START;
				//AppendMenu(hPopupRecent,
				//	MF_BYCOMMAND,
				//	cmd,
				//	L"Dummy");
				//if (!gbNoIcon)
				//{
				//	makeOwnerDraw(hMenu, (UINT)(UINT_PTR)hPopupRecent);
				//}
				//gPopupMap[hPopupRecent] = stdGetModuleFileName();
			}
		}
	}
	break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

wstring getMessageTitleString()
{
	return stdFormat(L"%s v%s (%s)",
		APPNAME,
		GetVersionString(nullptr, 3).c_str(),
		Is64BitProcess() ? L"x64" : L"x86");
}
void OpenAndWait(HWND hWnd, LPCWSTR pShortcut)
{
	CKernelHandle processHandle;
	if (OpenCommon(hWnd, pShortcut, NULL, NULL, &processHandle))
	{
		WaitForInputIdle(processHandle, WAIT_FOR_PROCESSIDLE);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	ghInst = hInstance;
#ifndef NDEBUG
	gsw = make_unique<wstop_watch>();
#endif

	i18nInitLangmap(hInstance, NULL, APPNAME);

	CKernelHandle singleMutex(CreateMutex(NULL, TRUE, L"ShowQLSingleInstance"));
	if (!singleMutex)
		ErrorExit(GetLastError());
	DWORD dwLastError = ::GetLastError();
	if (dwLastError == ERROR_ALREADY_EXISTS)
	{
		return -1;
	}

	InitHighDPISupport();

	Profile::CHashIni ini(Profile::ReadAll(GetIniPath()));
	{
		Profile::GetBool(SECTION_OPTION, KEY_SHOW_HIDDEN, false, gbShowHidden, ini);
		Profile::GetBool(SECTION_OPTION, KEY_NO_ICON, false, gbNoIcon, ini);
	}

	CCommandLineParser parser(I18N(L"Show QuickLaunch Menus"), APPNAME);

	bool bVersion = false;
	parser.AddOptionRange({ L"-v",L"-version",L"--version" }, 0, &bVersion, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows version"));

	bool bHelp = false;
	parser.AddOptionRange({ L"-h",L"/?",L"-help",L"--help" }, 0, &bHelp, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows Help"));

	bool bExplorer = false;
	parser.AddOptionRange({ L"-e",L"--explorer" }, 0, &bExplorer, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows in Explorer (Press SHIFT in startup)"));

	bool bPinMe = false;
	parser.AddOptionRange({ L"-p",L"--pin-me" }, 0, &bPinMe, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows MessageBox for pinning this app."));

	parser.AddOptionRange({ L"-ni",L"--no-icon" }, 0, &gbNoIcon, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows no icons"));

	parser.AddOptionRange({ L"-sh",L"--show-hidden" }, 0, &gbShowHidden, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Shows hidden directories"));


	COption mainArgs(L"", ArgCount::ArgCount_One, ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directory to show in menu"));
	parser.AddOption(&mainArgs);

	parser.Parse();

	if (parser.hadUnknownOption())
	{
		ErrorExit(stdFormat(I18N(L"Unknown option:%s"),
			parser.getUnknowOptionStrings().c_str()));
	}
	if (bVersion)
	{
		wstring message = stdFormat(L"%s v%s",
			APPNAME, GetVersionString(nullptr, 3).c_str()).c_str();
		message += L"\r\n\r\n";
		message += L"Gitrev:\r\n";
		message += stdStringReplace(GITREV::GetHashMessage(),
			L"\n", L"\r\n");
		MessageBox(nullptr,
			message.c_str(),
			getMessageTitleString().c_str(),
			MB_ICONINFORMATION);
		return 0;
	}
	if (bHelp)
	{
		MessageBox(nullptr,
			parser.getHelpMessage().c_str(),
			getMessageTitleString().c_str(),
			MB_ICONINFORMATION);
		return 0;
	}
	if (bPinMe)
	{
		MessageBox(nullptr,
			I18N(L"Please pin this in the taskbar."),
			getMessageTitleString().c_str(),
			MB_ICONINFORMATION);
		return 0;
	}

	TRACE_STOPWATCH(L"Started");

	if (mainArgs.getValueCount() > 1)
	{
		ErrorExit(I18N(L"Only one main argument is acceptable"));
	}
	CcoInitializer coIniter;
	CHWnd wnd(CreateSimpleWindow(WndProc));

	if (!gbNoIcon)
	{
		constexpr int heihtDelta = 2;
		gIconWidth = GetSystemMetrics(SM_CXSMICON);
		gIconHeight = GetSystemMetrics(SM_CYSMICON);
		gItemHeight = gIconHeight + (2 * heihtDelta);
		gItemDeltaX = 4;
		gItemDeltaY = heihtDelta;

		NONCLIENTMETRICS ncm = { 0 };
		ncm.cbSize = sizeof(ncm);
		if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
			ErrorExit(GetLastError());
		gMenuFont = CreateFontIndirect(&ncm.lfMenuFont);
	}

	ghPopup = CreatePopupMenu();
	gPopupMap[ghPopup] = wstring();
	InsertMenu(ghPopup, 0, MF_BYCOMMAND, MENUID_DUMMY, L"<DUMMY>");

	POINT curPos;
	GetCursorPos(&curPos);
	SetForegroundWindow(wnd);
	TRACE_STOPWATCH(L"Before TrackPopup");
	const UINT cmd = TrackPopupMenu(ghPopup,
		TPM_RETURNCMD,
		curPos.x, curPos.y,
		0,
		wnd,
		NULL);
	TRACE_STOPWATCH(L"After TrackPopup");
	DVERIFY(singleMutex.Close());
	if (false)
	{
	}
	else if (cmd == MENUID_OPTIONS)
	{
		wstring appOption = stdCombinePath(
			stdGetParentDirectory(stdGetModuleFileName()),
			L"ShowQLOption.exe");
		OpenAndWait(wnd, appOption.c_str());
	}
	else if (gCmdMap.find(cmd) != gCmdMap.end())
	{
		BringWinTop(gCmdMap[cmd]);
	}
	return 0;
}