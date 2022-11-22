#pragma once

#define SECTION_OPTION "Option"
#define SECTION_RECENTS "Recents"

#define KEY_SHOW_HIDDEN "ShowHidden"
#define KEY_NO_ICON "NoIcons"
#define KEY_NO_RECENTITEMS "NoRecentItems"
#define KEY_RECENT_ITEMS "RecentItems"
#define KEY_RECENTITEMCOUNT "RecentItemCount"

std::wstring GetIniPath();
std::wstring GetWindowTextString(HWND hWnd);