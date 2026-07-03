#include "settings.h"

static const wchar_t* kRegPath = L"Software\\SimpleCalc";
static const wchar_t* kValueName = L"WindowPlacement";

BOOL Settings_LoadPlacement(WINDOWPLACEMENT* outPlacement) {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return FALSE;
    }

    WINDOWPLACEMENT wp;
    DWORD size = sizeof(wp);
    DWORD type = REG_BINARY;
    LONG result = RegQueryValueExW(key, kValueName, NULL, &type, (LPBYTE)&wp, &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(wp) ||
        wp.length != sizeof(wp)) {
        return FALSE;
    }

    *outPlacement = wp;
    return TRUE;
}

void Settings_SavePlacement(HWND hwnd) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    if (!GetWindowPlacement(hwnd, &wp)) return;

    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegPath, 0, NULL, 0, KEY_WRITE, NULL,
                         &key, NULL) != ERROR_SUCCESS) {
        return;
    }
    RegSetValueExW(key, kValueName, 0, REG_BINARY, (const BYTE*)&wp, sizeof(wp));
    RegCloseKey(key);
}
