#include "Utility.h"
#include <windows.h>

std::wstring GetForegroundWindowTitle() {
    wchar_t wndTitle[256];
    HWND foreground = GetForegroundWindow();
    GetWindowTextW(foreground, wndTitle, sizeof(wndTitle) / sizeof(wchar_t));
    return std::wstring(wndTitle);
}
