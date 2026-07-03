#include "calc_expr.h"
#include "expr_parser.h"
#include "numfmt.h"
#include <wctype.h>

static double CurrentLiveValue(ExprCalcState* s) {
    double v;
    if (EvalExpression(s->buffer, s->degMode, &v)) return v;
    return 0.0;
}

static void UpdateLivePreview(ExprCalcState* s) {
    double v;
    if (wcslen(s->buffer) > 0 && EvalExpression(s->buffer, s->degMode, &v)) {
        FormatNumber(v, s->resultLine, ARRAYSIZE(s->resultLine));
    } else {
        s->resultLine[0] = L'\0';
    }
}

void ExprCalc_Reset(ExprCalcState* s) {
    s->buffer[0] = L'\0';
    s->historyLine[0] = L'\0';
    s->resultLine[0] = L'\0';
    s->degMode = TRUE;
    s->secondActive = FALSE;
    s->hasError = FALSE;
    s->justEvaluated = FALSE;
    s->memory = 0.0;
    s->memoryHasValue = FALSE;
}

void ExprCalc_Append(ExprCalcState* s, const wchar_t* text, BOOL continueAfterResult) {
    if (s->hasError) {
        s->buffer[0] = L'\0';
        s->historyLine[0] = L'\0';
        s->hasError = FALSE;
        s->justEvaluated = FALSE;
    } else if (s->justEvaluated) {
        if (!continueAfterResult) {
            s->buffer[0] = L'\0';
            s->historyLine[0] = L'\0';
        }
        s->justEvaluated = FALSE;
    }

    size_t curLen = wcslen(s->buffer);
    size_t addLen = wcslen(text);
    if (curLen + addLen < ARRAYSIZE(s->buffer)) {
        wcscat_s(s->buffer, ARRAYSIZE(s->buffer), text);
    }

    UpdateLivePreview(s);
}

static void WrapBuffer(ExprCalcState* s, const wchar_t* prefix, const wchar_t* suffix) {
    if (wcslen(s->buffer) == 0) return;
    if (s->hasError) return;

    wchar_t tmp[256];
    int written = swprintf_s(tmp, ARRAYSIZE(tmp), L"%s%s%s", prefix, s->buffer, suffix);
    if (written > 0) {
        wcscpy_s(s->buffer, ARRAYSIZE(s->buffer), tmp);
    }
    s->justEvaluated = FALSE;
    UpdateLivePreview(s);
}

void ExprCalc_WrapReciprocal(ExprCalcState* s) { WrapBuffer(s, L"1/(", L")"); }
void ExprCalc_WrapSquare(ExprCalcState* s) { WrapBuffer(s, L"(", L")^2"); }

void ExprCalc_WrapCube(ExprCalcState* s) {
    if (s->secondActive) {
        WrapBuffer(s, L"(", L")^(1/3)");
    } else {
        WrapBuffer(s, L"(", L")^3");
    }
}

void ExprCalc_ToggleSign(ExprCalcState* s) {
    if (s->hasError) return;
    size_t len = wcslen(s->buffer);
    if (len == 0) return;

    if (len >= 3 && wcsncmp(s->buffer, L"-(", 2) == 0 && s->buffer[len - 1] == L')') {
        wchar_t tmp[ARRAYSIZE(s->buffer)];
        wcsncpy_s(tmp, ARRAYSIZE(tmp), s->buffer + 2, len - 3);
        wcscpy_s(s->buffer, ARRAYSIZE(s->buffer), tmp);
        s->justEvaluated = FALSE;
        UpdateLivePreview(s);
    } else {
        WrapBuffer(s, L"-(", L")");
    }
}

void ExprCalc_Backspace(ExprCalcState* s) {
    if (s->hasError || s->justEvaluated) {
        ExprCalc_Clear(s);
        return;
    }
    size_t len = wcslen(s->buffer);
    if (len > 0) s->buffer[len - 1] = L'\0';
    UpdateLivePreview(s);
}

void ExprCalc_Clear(ExprCalcState* s) {
    s->buffer[0] = L'\0';
    s->historyLine[0] = L'\0';
    s->resultLine[0] = L'\0';
    s->hasError = FALSE;
    s->justEvaluated = FALSE;
}

void ExprCalc_ClearEntry(ExprCalcState* s) {
    if (s->hasError) { ExprCalc_Clear(s); return; }

    size_t len = wcslen(s->buffer);
    size_t trimTo = len;
    while (trimTo > 0 && (iswdigit(s->buffer[trimTo - 1]) || s->buffer[trimTo - 1] == L'.')) {
        trimTo--;
    }

    if (trimTo == len) {
        ExprCalc_Clear(s);
        return;
    }

    s->buffer[trimTo] = L'\0';
    UpdateLivePreview(s);
}

void ExprCalc_Evaluate(ExprCalcState* s) {
    if (s->hasError || wcslen(s->buffer) == 0) return;

    wcscpy_s(s->historyLine, ARRAYSIZE(s->historyLine), s->buffer);

    double result;
    if (!EvalExpression(s->buffer, s->degMode, &result)) {
        s->hasError = TRUE;
        wcscpy_s(s->buffer, ARRAYSIZE(s->buffer), L"Error");
        wcscpy_s(s->resultLine, ARRAYSIZE(s->resultLine), L"Error");
        return;
    }

    wchar_t numBuf[64];
    FormatNumber(result, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->buffer, ARRAYSIZE(s->buffer), numBuf);
    wcscpy_s(s->resultLine, ARRAYSIZE(s->resultLine), numBuf);
    s->justEvaluated = TRUE;
}

void ExprCalc_MC(ExprCalcState* s) {
    s->memory = 0.0;
    s->memoryHasValue = FALSE;
}

void ExprCalc_MR(ExprCalcState* s) {
    wchar_t numBuf[64];
    FormatNumber(s->memory, numBuf, ARRAYSIZE(numBuf));
    ExprCalc_Append(s, numBuf, FALSE);
}

void ExprCalc_MPlus(ExprCalcState* s) {
    s->memory += CurrentLiveValue(s);
    s->memoryHasValue = TRUE;
}

void ExprCalc_MMinus(ExprCalcState* s) {
    s->memory -= CurrentLiveValue(s);
    s->memoryHasValue = TRUE;
}

void ExprCalc_MS(ExprCalcState* s) {
    s->memory = CurrentLiveValue(s);
    s->memoryHasValue = TRUE;
}

void ExprCalc_ToggleSecond(ExprCalcState* s) {
    s->secondActive = !s->secondActive;
}

void ExprCalc_ToggleDegRad(ExprCalcState* s) {
    s->degMode = !s->degMode;
    UpdateLivePreview(s);
}
