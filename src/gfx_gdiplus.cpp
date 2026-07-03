#include "gfx.h"

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;

static ULONG_PTR g_gdiplusToken = 0;

BOOL Gfx_Init(void) {
    /* gdiplus.dll is delay-loaded (see build.bat) so DLL search order can
       be locked to System32 at startup; this is the first call that
       actually triggers the load, so catch a delay-load failure here
       instead of letting it crash the process. */
    __try {
        GdiplusStartupInput input;
        Status s = GdiplusStartup(&g_gdiplusToken, &input, NULL);
        return s == Ok;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
}

void Gfx_Shutdown(void) {
    if (g_gdiplusToken) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

static Color ToGdipColor(COLORREF c) {
    return Color(255, GetRValue(c), GetGValue(c), GetBValue(c));
}

static void BuildRoundRectPath(GraphicsPath* path, const RECT* rc, int radius) {
    int d = radius * 2;
    int x = rc->left, y = rc->top;
    int w = rc->right - rc->left, h = rc->bottom - rc->top;

    if (d > w) d = w;
    if (d > h) d = h;

    if (d <= 0) {
        path->AddRectangle(Rect(x, y, w, h));
        return;
    }

    path->AddArc(x, y, d, d, 180.0f, 90.0f);
    path->AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path->AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path->AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path->CloseFigure();
}

void Gfx_FillRoundRect(HDC hdc, const RECT* rc, int radiusPx, COLORREF color) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    BuildRoundRectPath(&path, rc, radiusPx);

    SolidBrush brush(ToGdipColor(color));
    g.FillPath(&brush, &path);
}

void Gfx_FillRect(HDC hdc, const RECT* rc, COLORREF color) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeNone);
    SolidBrush brush(ToGdipColor(color));
    g.FillRectangle(&brush, rc->left, rc->top, rc->right - rc->left,
                     rc->bottom - rc->top);
}

void Gfx_FillLine(HDC hdc, int x, int y, int w, int h, COLORREF color) {
    RECT rc = { x, y, x + w, y + h };
    Gfx_FillRect(hdc, &rc, color);
}

void Gfx_DrawText(HDC hdc, const RECT* rc, const wchar_t* text,
                   const wchar_t* fontFamily, float fontSizePx, BOOL bold,
                   COLORREF color, int hAlign, int vAlign) {
    Graphics g(hdc);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    FontFamily family(fontFamily);
    FontFamily fallback(L"Segoe UI");
    BOOL useFallback = (family.GetLastStatus() != Ok);

    Font font(useFallback ? &fallback : &family, fontSizePx,
              bold ? FontStyleBold : FontStyleRegular, UnitPixel);

    SolidBrush brush(ToGdipColor(color));

    StringFormat fmt;
    fmt.SetAlignment(hAlign == GFX_ALIGN_CENTER ? StringAlignmentCenter
                      : hAlign == GFX_ALIGN_FAR  ? StringAlignmentFar
                                                   : StringAlignmentNear);
    fmt.SetLineAlignment(vAlign == GFX_ALIGN_CENTER ? StringAlignmentCenter
                          : vAlign == GFX_ALIGN_FAR  ? StringAlignmentFar
                                                       : StringAlignmentNear);
    fmt.SetTrimming(StringTrimmingNone);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);

    RectF layout((REAL)rc->left, (REAL)rc->top,
                 (REAL)(rc->right - rc->left), (REAL)(rc->bottom - rc->top));
    g.DrawString(text, -1, &font, layout, &fmt, &brush);
}
