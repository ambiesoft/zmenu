#include "../../lsMisc/stdosd/stdosd.h"
#include "common.h"

using namespace Ambiesoft::stdosd;

std::wstring GetIniPath() 
{
	return stdCombinePath(
		stdGetParentDirectory(stdGetModuleFileName()),
		L"ShowQL.ini");
}
std::wstring GetWindowTextString(HWND hWnd)
{
	if (!::IsWindow(hWnd))
		return std::wstring();

	int nLen = GetWindowTextLength(hWnd);
	std::vector<wchar_t> buff;
	buff.reserve(nLen + 1);

	if (nLen != GetWindowText(hWnd, buff.data(), nLen + 1))
		return std::wstring();

	return buff.data();
}