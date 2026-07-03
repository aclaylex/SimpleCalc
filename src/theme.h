#pragma once
#include "common.h"

typedef struct {
    BOOL isDark;

    COLORREF background;

    COLORREF displayText;
    COLORREF exprText;

    COLORREF numberBtnBg;
    COLORREF numberBtnBgHover;
    COLORREF numberBtnBgPressed;
    COLORREF numberBtnText;

    COLORREF funcBtnBg;
    COLORREF funcBtnBgHover;
    COLORREF funcBtnBgPressed;
    COLORREF funcBtnText;

    COLORREF accentBg;
    COLORREF accentBgHover;
    COLORREF accentBgPressed;
    COLORREF accentText;

    COLORREF toggleActiveBg;
    COLORREF toggleActiveText;

    COLORREF tabActiveText;
    COLORREF tabInactiveText;
    COLORREF tabIndicator;
} Theme;

BOOL Theme_IsSystemDark(void);
void Theme_Load(Theme* theme, BOOL dark);
