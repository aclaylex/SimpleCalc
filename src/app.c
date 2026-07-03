#include "app.h"
#include "theme.h"
#include "gfx.h"
#include "layout.h"
#include "calc_standard.h"
#include "calc_expr.h"
#include "resource.h"
#include "settings.h"

#include <windowsx.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

/* ---- base geometry, in 96-DPI logical pixels -------------------------- */
#define PAD_OUTER     14
#define PAD_GRID      8
#define HEADER_H      42
#define CORNER_R_MIN  6
#define CORNER_R_MAX  20

#define NOMINAL_CELL  58   /* used only to pick the initial window size */
#define NOMINAL_DISPLAY 130
#define MIN_CELL      34   /* smallest a button is allowed to shrink to */
#define MIN_DISPLAY_H 90
#define MAX_DISPLAY_H 220

typedef enum { MODE_STANDARD, MODE_SCIENTIFIC } AppMode;
typedef enum { TAB_NONE, TAB_STANDARD, TAB_SCIENTIFIC } TabHit;

typedef struct {
    HWND hwnd;
    UINT dpi;
    Theme theme;
    AppMode mode;
    StdCalcState std;
    ExprCalcState expr;
    int hoverIndex;
    int pressIndex;
    TabHit pressTab;
    BOOL tracking;
} AppState;

static AppState g_app;

static int Scale(AppState* a, int v) { return MulDiv(v, (int)a->dpi, USER_DEFAULT_SCREEN_DPI); }

static const Layout* LayoutForMode(AppMode mode) {
    return mode == MODE_STANDARD ? Layout_Standard() : Layout_Scientific();
}

static const Layout* CurrentLayout(AppState* a) { return LayoutForMode(a->mode); }

static void ApplyDarkTitleBar(HWND hwnd, BOOL dark) {
    BOOL value = dark;
    /* dwmapi.dll is present on every supported OS, but this is cosmetic
       only, so any failure (missing/blocked DLL) is swallowed rather than
       risking a crash over a title-bar color. */
    __try {
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

/* ---- responsive geometry ------------------------------------------------*/

static int ComputeDisplayHeight(AppState* a, int clientHeight) {
    int minD = Scale(a, MIN_DISPLAY_H);
    int maxD = Scale(a, MAX_DISPLAY_H);
    int suggested = (int)(clientHeight * 0.22);
    if (suggested < minD) suggested = minD;
    if (suggested > maxD) suggested = maxD;
    return suggested;
}

static RECT HeaderRect(AppState* a, RECT client) {
    RECT r = { 0, 0, client.right, Scale(a, HEADER_H) };
    return r;
}

static RECT DisplayRect(AppState* a, RECT client) {
    int top = Scale(a, HEADER_H);
    int h = ComputeDisplayHeight(a, client.bottom);
    RECT r = { 0, top, client.right, top + h };
    return r;
}

static RECT GridArea(AppState* a, RECT client) {
    int outer = Scale(a, PAD_OUTER);
    int header = Scale(a, HEADER_H);
    int display = ComputeDisplayHeight(a, client.bottom);
    RECT r = { outer, header + display + outer, client.right - outer, client.bottom - outer };
    if (r.right < r.left) r.right = r.left;
    if (r.bottom < r.top) r.bottom = r.top;
    return r;
}

/* Buttons stretch to fill whatever space is available, so the whole grid
   (and the display above it) follows window resizing. */
static RECT ButtonRect(AppState* a, RECT client, int row, int col) {
    const Layout* l = CurrentLayout(a);
    RECT grid = GridArea(a, client);
    int gap = Scale(a, PAD_GRID);

    int gridW = grid.right - grid.left;
    int gridH = grid.bottom - grid.top;
    int cellW = (gridW - (l->cols - 1) * gap) / l->cols;
    int cellH = (gridH - (l->rows - 1) * gap) / l->rows;
    if (cellW < 1) cellW = 1;
    if (cellH < 1) cellH = 1;

    int x = grid.left + col * (cellW + gap);
    int y = grid.top + row * (cellH + gap);
    RECT r = { x, y, x + cellW, y + cellH };
    return r;
}

static SIZE ComputeInitialClientSize(AppState* a) {
    const Layout* l = Layout_Standard();
    int cell = Scale(a, NOMINAL_CELL);
    int gap = Scale(a, PAD_GRID);
    int outer = Scale(a, PAD_OUTER);
    int header = Scale(a, HEADER_H);
    int display = Scale(a, NOMINAL_DISPLAY);

    SIZE sz;
    sz.cx = outer * 2 + l->cols * cell + (l->cols - 1) * gap;
    sz.cy = header + display + outer + l->rows * cell + (l->rows - 1) * gap + outer;
    return sz;
}

static SIZE ComputeMinClientSizeForMode(AppState* a, AppMode mode) {
    const Layout* l = LayoutForMode(mode);
    int cell = Scale(a, MIN_CELL);
    int gap = Scale(a, PAD_GRID);
    int outer = Scale(a, PAD_OUTER);
    int header = Scale(a, HEADER_H);
    int display = Scale(a, MIN_DISPLAY_H);

    SIZE sz;
    sz.cx = outer * 2 + l->cols * cell + (l->cols - 1) * gap;
    sz.cy = header + display + outer + l->rows * cell + (l->rows - 1) * gap + outer;
    return sz;
}

static void SetClientSize(AppState* a, SIZE sz) {
    RECT r = { 0, 0, sz.cx, sz.cy };
    DWORD style = (DWORD)GetWindowLongPtrW(a->hwnd, GWL_STYLE);
    DWORD exStyle = (DWORD)GetWindowLongPtrW(a->hwnd, GWL_EXSTYLE);
    AdjustWindowRectEx(&r, style, FALSE, exStyle);
    SetWindowPos(a->hwnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/* If the window is currently smaller than the new mode's minimum, grow it
   just enough to fit; never shrinks a window the user sized themselves. */
static void GrowWindowIfBelowModeMinimum(AppState* a, AppMode mode) {
    RECT cur;
    GetClientRect(a->hwnd, &cur);
    SIZE minSz = ComputeMinClientSizeForMode(a, mode);

    int newW = cur.right;
    int newH = cur.bottom;
    if (minSz.cx > newW) newW = minSz.cx;
    if (minSz.cy > newH) newH = minSz.cy;

    if (newW != cur.right || newH != cur.bottom) {
        SIZE sz = { newW, newH };
        SetClientSize(a, sz);
    }
}

/* Resolves the label/action a button currently shows, accounting for the
   "2nd" toggle (sin<->asin etc.) and the live DEG/RAD readout. */
static void ResolveButton(AppState* a, const ButtonDef* b, const wchar_t** outLabel,
                           ButtonAction* outAction) {
    BOOL second = (a->mode == MODE_SCIENTIFIC) && a->expr.secondActive;
    if (second && b->altLabel) {
        *outLabel = b->altLabel;
        *outAction = (b->altAction != ACT_NONE) ? b->altAction : b->action;
    } else {
        *outLabel = b->label;
        *outAction = b->action;
    }
    if (b->action == ACT_DEGRAD) {
        *outLabel = a->expr.degMode ? L"DEG" : L"RAD";
    }
}

static void Dispatch(AppState* a, ButtonAction action, wchar_t ch) {
    if (a->mode == MODE_STANDARD) {
        StdCalcState* s = &a->std;
        switch (action) {
            case ACT_DIGIT:        StdCalc_Digit(s, ch); break;
            case ACT_DECIMAL:      StdCalc_Decimal(s); break;
            case ACT_OP:           StdCalc_Operator(s, ch); break;
            case ACT_EQUALS:       StdCalc_Equals(s); break;
            case ACT_CLEAR:        StdCalc_Clear(s); break;
            case ACT_CLEAR_ENTRY:  StdCalc_ClearEntry(s); break;
            case ACT_BACKSPACE:    StdCalc_Backspace(s); break;
            case ACT_SIGN:         StdCalc_Sign(s); break;
            case ACT_PERCENT:      StdCalc_Percent(s); break;
            case ACT_SQRT:         StdCalc_Sqrt(s); break;
            case ACT_SQUARE:       StdCalc_Square(s); break;
            case ACT_RECIP:        StdCalc_Reciprocal(s); break;
            case ACT_MC:           StdCalc_MC(s); break;
            case ACT_MR:           StdCalc_MR(s); break;
            case ACT_MPLUS:        StdCalc_MPlus(s); break;
            case ACT_MMINUS:       StdCalc_MMinus(s); break;
            case ACT_MS:           StdCalc_MS(s); break;
            default: break;
        }
    } else {
        ExprCalcState* s = &a->expr;
        wchar_t buf[4];
        switch (action) {
            case ACT_DIGIT:        buf[0] = ch; buf[1] = 0; ExprCalc_Append(s, buf, FALSE); break;
            case ACT_DECIMAL:      ExprCalc_Append(s, L".", FALSE); break;
            case ACT_OP: {
                wchar_t pretty = (ch == L'-') ? 0x2212 : (ch == L'*') ? 0x00D7
                                : (ch == L'/') ? 0x00F7 : ch;
                buf[0] = pretty; buf[1] = 0;
                ExprCalc_Append(s, buf, TRUE);
                break;
            }
            case ACT_PERCENT:      ExprCalc_Append(s, L"%", TRUE); break;
            case ACT_PAREN_OPEN:   ExprCalc_Append(s, L"(", TRUE); break;
            case ACT_PAREN_CLOSE:  ExprCalc_Append(s, L")", TRUE); break;
            case ACT_POW:          ExprCalc_Append(s, L"^", TRUE); break;
            case ACT_PI:           ExprCalc_Append(s, L"π", FALSE); break;
            case ACT_E:            ExprCalc_Append(s, L"e", FALSE); break;
            case ACT_SIN:          ExprCalc_Append(s, L"sin(", FALSE); break;
            case ACT_COS:          ExprCalc_Append(s, L"cos(", FALSE); break;
            case ACT_TAN:          ExprCalc_Append(s, L"tan(", FALSE); break;
            case ACT_ASIN:         ExprCalc_Append(s, L"asin(", FALSE); break;
            case ACT_ACOS:         ExprCalc_Append(s, L"acos(", FALSE); break;
            case ACT_ATAN:         ExprCalc_Append(s, L"atan(", FALSE); break;
            case ACT_LN:           ExprCalc_Append(s, L"ln(", FALSE); break;
            case ACT_LOG:          ExprCalc_Append(s, L"log(", FALSE); break;
            case ACT_SQRT:         ExprCalc_Append(s, L"sqrt(", FALSE); break;
            case ACT_EXP:          ExprCalc_Append(s, L"e^(", FALSE); break;
            case ACT_POW10:        ExprCalc_Append(s, L"10^(", FALSE); break;
            case ACT_RECIP:        ExprCalc_WrapReciprocal(s); break;
            case ACT_SQUARE:       ExprCalc_WrapSquare(s); break;
            case ACT_CUBE:         ExprCalc_WrapCube(s); break;
            case ACT_SIGN:         ExprCalc_ToggleSign(s); break;
            case ACT_EQUALS:       ExprCalc_Evaluate(s); break;
            case ACT_CLEAR:        ExprCalc_Clear(s); break;
            case ACT_CLEAR_ENTRY:  ExprCalc_ClearEntry(s); break;
            case ACT_BACKSPACE:    ExprCalc_Backspace(s); break;
            case ACT_MC:           ExprCalc_MC(s); break;
            case ACT_MR:           ExprCalc_MR(s); break;
            case ACT_MPLUS:        ExprCalc_MPlus(s); break;
            case ACT_MMINUS:       ExprCalc_MMinus(s); break;
            case ACT_MS:           ExprCalc_MS(s); break;
            case ACT_SECOND:       ExprCalc_ToggleSecond(s); break;
            case ACT_DEGRAD:       ExprCalc_ToggleDegRad(s); break;
            default: break;
        }
    }
    InvalidateRect(a->hwnd, NULL, FALSE);
}

/* ---- painting ----------------------------------------------------------*/

static void GetDisplayStrings(AppState* a, const wchar_t** topLine, const wchar_t** bottomLine) {
    static wchar_t topBuf[320];
    if (a->mode == MODE_STANDARD) {
        *topLine = a->std.exprLine;
        *bottomLine = a->std.entry;
    } else {
        ExprCalcState* e = &a->expr;
        if (e->hasError) {
            *topLine = e->historyLine;
            *bottomLine = e->buffer;
        } else if (e->justEvaluated) {
            swprintf_s(topBuf, ARRAYSIZE(topBuf), L"%s =", e->historyLine);
            *topLine = topBuf;
            *bottomLine = e->buffer;
        } else if (wcslen(e->buffer) == 0) {
            *topLine = L"";
            *bottomLine = L"0";
        } else {
            *topLine = e->resultLine;
            *bottomLine = e->buffer;
        }
    }
}

static float BigFontSizeForLength(AppState* a, size_t len, int displayHeight) {
    float base = (float)displayHeight * 0.42f;
    float minSz = (float)Scale(a, 18);
    float maxSz = (float)Scale(a, 64);
    if (base < minSz) base = minSz;
    if (base > maxSz) base = maxSz;

    if (len > 14) return base * 0.5f;
    if (len > 10) return base * 0.65f;
    if (len > 7)  return base * 0.8f;
    return base;
}

static void DrawHeader(AppState* a, HDC hdc, RECT client) {
    RECT hr = HeaderRect(a, client);
    int halfW = (hr.right - hr.left) / 2;

    RECT leftTab = { hr.left, hr.top, hr.left + halfW, hr.bottom };
    RECT rightTab = { hr.left + halfW, hr.top, hr.right, hr.bottom };

    COLORREF leftColor = (a->mode == MODE_STANDARD) ? a->theme.tabActiveText : a->theme.tabInactiveText;
    COLORREF rightColor = (a->mode == MODE_SCIENTIFIC) ? a->theme.tabActiveText : a->theme.tabInactiveText;

    Gfx_DrawText(hdc, &leftTab, L"Standard", L"Segoe UI Variable Text",
                 (float)Scale(a, 14), a->mode == MODE_STANDARD, leftColor,
                 GFX_ALIGN_CENTER, GFX_ALIGN_CENTER);
    Gfx_DrawText(hdc, &rightTab, L"Scientific", L"Segoe UI Variable Text",
                 (float)Scale(a, 14), a->mode == MODE_SCIENTIFIC, rightColor,
                 GFX_ALIGN_CENTER, GFX_ALIGN_CENTER);

    int indicatorH = Scale(a, 3);
    RECT activeTab = (a->mode == MODE_STANDARD) ? leftTab : rightTab;
    Gfx_FillLine(hdc, activeTab.left + Scale(a, 16), hr.bottom - indicatorH,
                 (activeTab.right - activeTab.left) - Scale(a, 32), indicatorH,
                 a->theme.tabIndicator);
}

static void DrawDisplay(AppState* a, HDC hdc, RECT client) {
    RECT dr = DisplayRect(a, client);
    int padRight = Scale(a, PAD_OUTER);
    int displayH = dr.bottom - dr.top;

    const wchar_t* topLine;
    const wchar_t* bottomLine;
    GetDisplayStrings(a, &topLine, &bottomLine);

    BOOL isError = (a->mode == MODE_STANDARD) ? a->std.hasError : a->expr.hasError;

    int topH = Scale(a, 32);
    if (topH > displayH / 3) topH = displayH / 3;

    RECT topRect = dr;
    topRect.left += padRight;
    topRect.right -= padRight;
    topRect.top = dr.top + Scale(a, 10);
    topRect.bottom = topRect.top + topH;

    RECT bottomRect = dr;
    bottomRect.left += padRight;
    bottomRect.right -= padRight;
    bottomRect.top = topRect.bottom;
    bottomRect.bottom = dr.bottom - Scale(a, 8);

    if (topLine[0]) {
        Gfx_DrawText(hdc, &topRect, topLine, L"Segoe UI Variable Small",
                     (float)Scale(a, 15), FALSE, a->theme.exprText,
                     GFX_ALIGN_FAR, GFX_ALIGN_FAR);
    }

    COLORREF bottomColor = isError ? RGB(0xE0, 0x5A, 0x4E) : a->theme.displayText;
    float fontSize = BigFontSizeForLength(a, wcslen(bottomLine), bottomRect.bottom - bottomRect.top);
    Gfx_DrawText(hdc, &bottomRect, bottomLine, L"Segoe UI Variable Display",
                 fontSize, FALSE, bottomColor, GFX_ALIGN_FAR, GFX_ALIGN_FAR);
}

static void DrawButtons(AppState* a, HDC hdc, RECT client) {
    const Layout* l = CurrentLayout(a);
    for (int i = 0; i < l->count; i++) {
        const ButtonDef* b = &l->buttons[i];
        RECT r = ButtonRect(a, client, b->row, b->col);
        int cellW = r.right - r.left;
        int cellH = r.bottom - r.top;
        int minCell = (cellW < cellH) ? cellW : cellH;

        const wchar_t* label;
        ButtonAction action;
        ResolveButton(a, b, &label, &action);
        (void)action;

        BOOL isActiveToggle = (b->action == ACT_SECOND && a->expr.secondActive);

        COLORREF bg, fg;
        if (isActiveToggle) {
            bg = a->theme.toggleActiveBg;
            fg = a->theme.toggleActiveText;
        } else if (b->isAccent) {
            bg = a->theme.accentBg;
            fg = a->theme.accentText;
        } else if (b->isFunc) {
            bg = a->theme.funcBtnBg;
            fg = a->theme.funcBtnText;
        } else {
            bg = a->theme.numberBtnBg;
            fg = a->theme.numberBtnText;
        }

        if (i == a->pressIndex) {
            bg = b->isAccent ? a->theme.accentBgPressed
                 : b->isFunc ? a->theme.funcBtnBgPressed
                             : a->theme.numberBtnBgPressed;
        } else if (i == a->hoverIndex && !isActiveToggle) {
            bg = b->isAccent ? a->theme.accentBgHover
                 : b->isFunc ? a->theme.funcBtnBgHover
                             : a->theme.numberBtnBgHover;
        }

        int radius = (int)(minCell * 0.16f);
        int radiusMin = Scale(a, CORNER_R_MIN);
        int radiusMax = Scale(a, CORNER_R_MAX);
        if (radius < radiusMin) radius = radiusMin;
        if (radius > radiusMax) radius = radiusMax;

        Gfx_FillRoundRect(hdc, &r, radius, bg);

        BOOL isLong = wcslen(label) > 2;
        float fontSize = minCell * (isLong ? 0.30f : 0.42f);
        float fontMin = (float)Scale(a, 10);
        float fontMax = (float)Scale(a, isLong ? 20 : 30);
        if (fontSize < fontMin) fontSize = fontMin;
        if (fontSize > fontMax) fontSize = fontMax;

        Gfx_DrawText(hdc, &r, label, L"Segoe UI Variable Text", fontSize, FALSE, fg,
                     GFX_ALIGN_CENTER, GFX_ALIGN_CENTER);
    }
}

static void Paint(AppState* a) {
    RECT client;
    GetClientRect(a->hwnd, &client);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(a->hwnd, &ps);

    /* A 0-area client rect (e.g. a hostile SetWindowPos to 0x0, or the
       window briefly mid-animation) makes CreateCompatibleBitmap fail;
       just skip painting that frame rather than risk drawing into a DC
       with no valid bitmap selected. */
    if (client.right > 0 && client.bottom > 0) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
        if (memDC && memBmp) {
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            HBRUSH bgBrush = CreateSolidBrush(a->theme.background);
            FillRect(memDC, &client, bgBrush);
            DeleteObject(bgBrush);

            DrawHeader(a, memDC, client);
            DrawDisplay(a, memDC, client);
            DrawButtons(a, memDC, client);

            BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
        }
        if (memBmp) DeleteObject(memBmp);
        if (memDC) DeleteDC(memDC);
    }

    EndPaint(a->hwnd, &ps);
}

/* ---- hit testing ---------------------------------------------------------*/

static int HitTestButton(AppState* a, RECT client, POINT pt) {
    const Layout* l = CurrentLayout(a);
    for (int i = 0; i < l->count; i++) {
        RECT r = ButtonRect(a, client, l->buttons[i].row, l->buttons[i].col);
        if (PtInRect(&r, pt)) return i;
    }
    return -1;
}

static TabHit HitTestTab(AppState* a, RECT client, POINT pt) {
    RECT hr = HeaderRect(a, client);
    if (pt.y < hr.top || pt.y >= hr.bottom) return TAB_NONE;
    int halfW = (hr.right - hr.left) / 2;
    return (pt.x < hr.left + halfW) ? TAB_STANDARD : TAB_SCIENTIFIC;
}

/* ---- window procedure -----------------------------------------------------*/

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_app.hwnd = hwnd;
            g_app.dpi = GetDpiForWindow(hwnd);
            BOOL dark = Theme_IsSystemDark();
            Theme_Load(&g_app.theme, dark);
            ApplyDarkTitleBar(hwnd, dark);
            StdCalc_Reset(&g_app.std);
            ExprCalc_Reset(&g_app.expr);
            g_app.mode = MODE_STANDARD;
            g_app.hoverIndex = -1;
            g_app.pressIndex = -1;
            g_app.pressTab = TAB_NONE;
            g_app.tracking = FALSE;
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            Paint(&g_app);
            return 0;

        case WM_SIZE:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_GETMINMAXINFO: {
            /* Normally OS-sent with a valid pointer, but any process on the
               desktop can PostMessage/SendMessage this with a spoofed
               lParam, so guard the dereference. */
            __try {
                if (g_app.dpi == 0) g_app.dpi = USER_DEFAULT_SCREEN_DPI;
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                SIZE minSz = ComputeMinClientSizeForMode(&g_app, g_app.mode);
                RECT r = { 0, 0, minSz.cx, minSz.cy };
                DWORD style = (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE);
                DWORD exStyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
                AdjustWindowRectEx(&r, style, FALSE, exStyle);
                mmi->ptMinTrackSize.x = r.right - r.left;
                mmi->ptMinTrackSize.y = r.bottom - r.top;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (!g_app.tracking) {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = 0;
                TrackMouseEvent(&tme);
                g_app.tracking = TRUE;
            }
            RECT client;
            GetClientRect(hwnd, &client);
            int idx = HitTestButton(&g_app, client, pt);
            if (idx != g_app.hoverIndex) {
                g_app.hoverIndex = idx;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            g_app.tracking = FALSE;
            if (g_app.hoverIndex != -1) {
                g_app.hoverIndex = -1;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            SetCapture(hwnd);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT client;
            GetClientRect(hwnd, &client);
            TabHit tab = HitTestTab(&g_app, client, pt);
            if (tab != TAB_NONE) {
                g_app.pressTab = tab;
                g_app.pressIndex = -1;
            } else {
                g_app.pressIndex = HitTestButton(&g_app, client, pt);
                g_app.pressTab = TAB_NONE;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONUP: {
            /* Snapshot press state before ReleaseCapture(): releasing
               capture synchronously re-enters this WndProc via
               WM_CAPTURECHANGED (this window is the one losing capture),
               whose handler clears pressTab/pressIndex. Reading g_app's
               fields after that point would always see "nothing pressed"
               and silently eat every click. */
            TabHit pressedTab = g_app.pressTab;
            int pressedIndex = g_app.pressIndex;
            ReleaseCapture();

            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT client;
            GetClientRect(hwnd, &client);

            if (pressedTab != TAB_NONE) {
                TabHit tab = HitTestTab(&g_app, client, pt);
                if (tab == pressedTab) {
                    AppMode newMode = (tab == TAB_STANDARD) ? MODE_STANDARD : MODE_SCIENTIFIC;
                    if (newMode != g_app.mode) {
                        g_app.mode = newMode;
                        g_app.hoverIndex = -1;
                        GrowWindowIfBelowModeMinimum(&g_app, newMode);
                    }
                }
            } else if (pressedIndex >= 0) {
                int idx = HitTestButton(&g_app, client, pt);
                if (idx == pressedIndex) {
                    const Layout* l = CurrentLayout(&g_app);
                    const ButtonDef* b = &l->buttons[idx];
                    const wchar_t* label;
                    ButtonAction action;
                    ResolveButton(&g_app, b, &label, &action);
                    (void)label;
                    Dispatch(&g_app, action, b->ch);
                }
            }

            g_app.pressTab = TAB_NONE;
            g_app.pressIndex = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_CAPTURECHANGED:
            /* Mouse capture was stolen mid-press (e.g. Alt+Tab while the
               button was held down); drop the pressed visual instead of
               leaving a button stuck looking pressed forever. */
            if (g_app.pressIndex != -1 || g_app.pressTab != TAB_NONE) {
                g_app.pressIndex = -1;
                g_app.pressTab = TAB_NONE;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_CHAR: {
            wchar_t c = (wchar_t)wParam;
            if (c >= L'0' && c <= L'9') Dispatch(&g_app, ACT_DIGIT, c);
            else if (c == L'.') Dispatch(&g_app, ACT_DECIMAL, 0);
            else if (c == L'+') Dispatch(&g_app, ACT_OP, L'+');
            else if (c == L'-') Dispatch(&g_app, ACT_OP, L'-');
            else if (c == L'*') Dispatch(&g_app, ACT_OP, L'*');
            else if (c == L'/') Dispatch(&g_app, ACT_OP, L'/');
            else if (c == L'\r' || c == L'=') Dispatch(&g_app, ACT_EQUALS, 0);
            else if (c == L'\b') Dispatch(&g_app, ACT_BACKSPACE, 0);
            else if (c == L'%') Dispatch(&g_app, ACT_PERCENT, 0);
            else if (c == L'(' && g_app.mode == MODE_SCIENTIFIC) Dispatch(&g_app, ACT_PAREN_OPEN, 0);
            else if (c == L')' && g_app.mode == MODE_SCIENTIFIC) Dispatch(&g_app, ACT_PAREN_CLOSE, 0);
            else if (c == L'^' && g_app.mode == MODE_SCIENTIFIC) Dispatch(&g_app, ACT_POW, 0);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) Dispatch(&g_app, ACT_CLEAR, 0);
            else if (wParam == VK_DELETE) Dispatch(&g_app, ACT_CLEAR_ENTRY, 0);
            return 0;

        case WM_SETTINGCHANGE: {
            /* lParam is normally a valid null-terminated string from the OS,
               but this message (like any other) can be spoofed by another
               process on the desktop with an arbitrary lParam, so the
               dereference is guarded rather than trusted. */
            BOOL isColorChange = FALSE;
            __try {
                if (lParam) {
                    isColorChange = (wcscmp((const wchar_t*)lParam, L"ImmersiveColorSet") == 0);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                isColorChange = FALSE;
            }
            if (isColorChange) {
                BOOL dark = Theme_IsSystemDark();
                Theme_Load(&g_app.theme, dark);
                ApplyDarkTitleBar(hwnd, dark);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_DPICHANGED: {
            g_app.dpi = HIWORD(wParam);
            __try {
                const RECT* suggested = (const RECT*)lParam;
                SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
                             suggested->right - suggested->left, suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            return 0;
        }

        case WM_DESTROY:
            Settings_SavePlacement(hwnd);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ---- entry points ---------------------------------------------------------*/

BOOL App_Init(HINSTANCE hInstance, int nCmdShow) {
    ZeroMemory(&g_app, sizeof(g_app));
    g_app.dpi = USER_DEFAULT_SCREEN_DPI;

    if (!Gfx_Init()) return FALSE;

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = L"SimpleCalcWindowClass";
    wc.hbrBackground = NULL;

    if (!RegisterClassExW(&wc)) return FALSE;

    DWORD style = WS_OVERLAPPEDWINDOW; /* resizable, minimizable, maximizable */
    RECT r = { 0, 0, 360, 560 };
    AdjustWindowRectEx(&r, style, FALSE, 0);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"SimpleCalc", style,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 r.right - r.left, r.bottom - r.top,
                                 NULL, NULL, hInstance, NULL);
    if (!hwnd) return FALSE;

    WINDOWPLACEMENT wp;
    BOOL haveSaved = Settings_LoadPlacement(&wp);
    if (haveSaved) {
        /* Clamp against this session's minimum (DPI/monitor may differ from
           when it was saved), and discard if it's off every monitor now. */
        SIZE minClient = ComputeMinClientSizeForMode(&g_app, MODE_STANDARD);
        RECT minWinRect = { 0, 0, minClient.cx, minClient.cy };
        AdjustWindowRectEx(&minWinRect, style, FALSE, 0);
        int minW = minWinRect.right - minWinRect.left;
        int minH = minWinRect.bottom - minWinRect.top;

        int w = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        int h = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        if (w < minW) wp.rcNormalPosition.right = wp.rcNormalPosition.left + minW;
        if (h < minH) wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + minH;

        if (wp.showCmd == SW_SHOWMINIMIZED) wp.showCmd = SW_SHOWNORMAL;

        if (MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONULL) == NULL) {
            haveSaved = FALSE;
        }
    }

    if (haveSaved) {
        SetWindowPlacement(hwnd, &wp);
        ShowWindow(hwnd, wp.showCmd);
    } else {
        SetClientSize(&g_app, ComputeInitialClientSize(&g_app));
        ShowWindow(hwnd, nCmdShow);
    }
    UpdateWindow(hwnd);
    return TRUE;
}

int App_Run(void) {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    Gfx_Shutdown();
    return (int)msg.wParam;
}
