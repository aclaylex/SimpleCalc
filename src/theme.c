#include "theme.h"

BOOL Theme_IsSystemDark(void) {
    HKEY key;
    const wchar_t* path =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return FALSE;
    }

    DWORD value = 1;
    DWORD size = sizeof(value);
    DWORD type = REG_DWORD;
    LONG result = RegQueryValueExW(key, L"AppsUseLightTheme", NULL, &type,
                                    (LPBYTE)&value, &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS) return FALSE;
    return value == 0; /* 0 = dark, 1 = light */
}

void Theme_Load(Theme* t, BOOL dark) {
    t->isDark = dark;

    if (dark) {
        t->background         = RGB(0x20, 0x20, 0x22);
        t->displayText         = RGB(0xF2, 0xF2, 0xF2);
        t->exprText             = RGB(0x9A, 0x9A, 0x9E);

        t->numberBtnBg          = RGB(0x2C, 0x2C, 0x2F);
        t->numberBtnBgHover     = RGB(0x38, 0x38, 0x3C);
        t->numberBtnBgPressed   = RGB(0x22, 0x22, 0x25);
        t->numberBtnText        = RGB(0xF2, 0xF2, 0xF2);

        t->funcBtnBg            = RGB(0x35, 0x35, 0x39);
        t->funcBtnBgHover       = RGB(0x41, 0x41, 0x46);
        t->funcBtnBgPressed     = RGB(0x2A, 0x2A, 0x2E);
        t->funcBtnText          = RGB(0xF2, 0xF2, 0xF2);

        t->accentBg              = RGB(0x4C, 0x8E, 0xFF);
        t->accentBgHover        = RGB(0x6B, 0xA3, 0xFF);
        t->accentBgPressed      = RGB(0x3A, 0x72, 0xD1);
        t->accentText            = RGB(0xFF, 0xFF, 0xFF);

        t->toggleActiveBg       = RGB(0x3A, 0x72, 0xD1);
        t->toggleActiveText     = RGB(0xFF, 0xFF, 0xFF);

        t->tabActiveText         = RGB(0xF2, 0xF2, 0xF2);
        t->tabInactiveText       = RGB(0x7A, 0x7A, 0x7E);
        t->tabIndicator          = RGB(0x4C, 0x8E, 0xFF);
    } else {
        t->background            = RGB(0xF3, 0xF3, 0xF3);
        t->displayText           = RGB(0x1B, 0x1B, 0x1B);
        t->exprText              = RGB(0x6E, 0x6E, 0x72);

        t->numberBtnBg           = RGB(0xFF, 0xFF, 0xFF);
        t->numberBtnBgHover      = RGB(0xEC, 0xEC, 0xEC);
        t->numberBtnBgPressed    = RGB(0xDA, 0xDA, 0xDA);
        t->numberBtnText         = RGB(0x1B, 0x1B, 0x1B);

        t->funcBtnBg             = RGB(0xE7, 0xE7, 0xE9);
        t->funcBtnBgHover        = RGB(0xDA, 0xDA, 0xDC);
        t->funcBtnBgPressed      = RGB(0xC8, 0xC8, 0xCB);
        t->funcBtnText           = RGB(0x1B, 0x1B, 0x1B);

        t->accentBg              = RGB(0x2F, 0x6F, 0xE4);
        t->accentBgHover         = RGB(0x4A, 0x86, 0xF2);
        t->accentBgPressed       = RGB(0x24, 0x59, 0xB8);
        t->accentText            = RGB(0xFF, 0xFF, 0xFF);

        t->toggleActiveBg        = RGB(0x2F, 0x6F, 0xE4);
        t->toggleActiveText      = RGB(0xFF, 0xFF, 0xFF);

        t->tabActiveText          = RGB(0x1B, 0x1B, 0x1B);
        t->tabInactiveText        = RGB(0x8A, 0x8A, 0x8E);
        t->tabIndicator           = RGB(0x2F, 0x6F, 0xE4);
    }
}
