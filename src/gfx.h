#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_ALIGN_NEAR   0
#define GFX_ALIGN_CENTER 1
#define GFX_ALIGN_FAR    2

BOOL Gfx_Init(void);
void Gfx_Shutdown(void);

/* Fills a rounded rectangle with a solid, anti-aliased color. */
void Gfx_FillRoundRect(HDC hdc, const RECT* rc, int radiusPx, COLORREF color);

/* Fills a plain anti-aliased rectangle (radius 0 fast-path helper). */
void Gfx_FillRect(HDC hdc, const RECT* rc, COLORREF color);

/* Draws a horizontal line, used for the tab-underline indicator. */
void Gfx_FillLine(HDC hdc, int x, int y, int w, int h, COLORREF color);

/* Draws anti-aliased, ClearType-rendered text. fontFamily should be a
   family that ships with Windows 11 (e.g. "Segoe UI Variable Display").
   fontSizePx is a device-pixel height (caller applies DPI scaling), so
   sizing stays consistent with the rest of the manually-scaled layout. */
void Gfx_DrawText(HDC hdc, const RECT* rc, const wchar_t* text,
                   const wchar_t* fontFamily, float fontSizePx, BOOL bold,
                   COLORREF color, int hAlign, int vAlign);

#ifdef __cplusplus
}
#endif
