#include "calc_standard.h"
#include "numfmt.h"

static wchar_t PrettyOp(wchar_t op) {
    switch (op) {
        case L'-': return 0x2212; /* minus sign */
        case L'*': return 0x00D7; /* multiplication sign */
        case L'/': return 0x00F7; /* division sign */
        default:   return op;
    }
}

static double ApplyOp(double a, double b, wchar_t op, BOOL* err) {
    switch (op) {
        case L'+': return a + b;
        case L'-': return a - b;
        case L'*': return a * b;
        case L'/':
            if (b == 0.0) { *err = TRUE; return 0.0; }
            return a / b;
        default:
            return b;
    }
}

double StdCalc_CurrentValue(const StdCalcState* s) {
    return wcstod(s->entry, NULL);
}

void StdCalc_Reset(StdCalcState* s) {
    s->accumulator = 0.0;
    s->lastOperand = 0.0;
    s->lastOp = 0;
    s->pendingOp = 0;
    s->startNewEntry = TRUE;
    s->hasError = FALSE;
    s->memory = 0.0;
    s->memoryHasValue = FALSE;
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"0");
    s->exprLine[0] = L'\0';
}

void StdCalc_Digit(StdCalcState* s, wchar_t digit) {
    if (s->hasError) StdCalc_Clear(s);

    if (s->startNewEntry) {
        s->entry[0] = digit;
        s->entry[1] = L'\0';
        s->startNewEntry = FALSE;
        return;
    }

    if (wcscmp(s->entry, L"0") == 0) {
        s->entry[0] = digit;
        s->entry[1] = L'\0';
        return;
    }

    size_t len = wcslen(s->entry);
    if (len < ARRAYSIZE(s->entry) - 1 && len < 16) {
        s->entry[len] = digit;
        s->entry[len + 1] = L'\0';
    }
}

void StdCalc_Decimal(StdCalcState* s) {
    if (s->hasError) StdCalc_Clear(s);

    if (s->startNewEntry) {
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"0.");
        s->startNewEntry = FALSE;
        return;
    }
    if (!wcschr(s->entry, L'.')) {
        size_t len = wcslen(s->entry);
        if (len < ARRAYSIZE(s->entry) - 1) {
            s->entry[len] = L'.';
            s->entry[len + 1] = L'\0';
        }
    }
}

void StdCalc_Operator(StdCalcState* s, wchar_t op) {
    if (s->hasError) return;

    double cur = StdCalc_CurrentValue(s);

    if (s->pendingOp != 0 && !s->startNewEntry) {
        BOOL err = FALSE;
        double result = ApplyOp(s->accumulator, cur, s->pendingOp, &err);
        if (err) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Cannot divide by zero");
            return;
        }
        if (isinf(result) || isnan(result)) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Overflow");
            return;
        }
        s->accumulator = result;
    } else if (s->pendingOp == 0) {
        s->accumulator = cur;
    }

    s->pendingOp = op;
    s->lastOp = 0;
    s->lastOperand = 0.0;
    s->startNewEntry = TRUE;

    wchar_t numBuf[64];
    FormatNumber(s->accumulator, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"%s %c", numBuf, PrettyOp(op));
}

void StdCalc_Equals(StdCalcState* s) {
    if (s->hasError) return;

    double cur = StdCalc_CurrentValue(s);

    if (s->pendingOp != 0) {
        wchar_t accBuf[64], curBuf[64];
        FormatNumber(s->accumulator, accBuf, ARRAYSIZE(accBuf));
        FormatNumber(cur, curBuf, ARRAYSIZE(curBuf));

        BOOL err = FALSE;
        double result = ApplyOp(s->accumulator, cur, s->pendingOp, &err);
        if (err) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Cannot divide by zero");
            return;
        }
        if (isinf(result) || isnan(result)) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Overflow");
            return;
        }

        swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"%s %c %s =", accBuf,
                   PrettyOp(s->pendingOp), curBuf);

        s->lastOp = s->pendingOp;
        s->lastOperand = cur;
        s->accumulator = result;
        s->pendingOp = 0;
    } else if (s->lastOp != 0) {
        BOOL err = FALSE;
        double result = ApplyOp(s->accumulator, s->lastOperand, s->lastOp, &err);
        if (err) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Cannot divide by zero");
            return;
        }
        if (isinf(result) || isnan(result)) {
            s->hasError = TRUE;
            wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Overflow");
            return;
        }
        s->accumulator = result;
    } else {
        return;
    }

    wchar_t numBuf[64];
    FormatNumber(s->accumulator, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_Percent(StdCalcState* s) {
    if (s->hasError) return;
    double cur = StdCalc_CurrentValue(s);
    double result = (s->pendingOp != 0) ? (s->accumulator * (cur / 100.0)) : (cur / 100.0);
    if (isinf(result) || isnan(result)) {
        s->hasError = TRUE;
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Overflow");
        return;
    }

    wchar_t oldBuf[64];
    wcscpy_s(oldBuf, ARRAYSIZE(oldBuf), s->entry);

    wchar_t numBuf[64];
    FormatNumber(result, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"%s%%", oldBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_Sqrt(StdCalcState* s) {
    if (s->hasError) return;
    double cur = StdCalc_CurrentValue(s);
    if (cur < 0.0) {
        s->hasError = TRUE;
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Invalid input");
        return;
    }

    wchar_t oldBuf[64];
    wcscpy_s(oldBuf, ARRAYSIZE(oldBuf), s->entry);

    double result = sqrt(cur);
    wchar_t numBuf[64];
    FormatNumber(result, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"√(%s)", oldBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_Square(StdCalcState* s) {
    if (s->hasError) return;
    double cur = StdCalc_CurrentValue(s);

    wchar_t oldBuf[64];
    wcscpy_s(oldBuf, ARRAYSIZE(oldBuf), s->entry);

    double result = cur * cur;
    if (isinf(result)) {
        s->hasError = TRUE;
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Overflow");
        return;
    }

    wchar_t numBuf[64];
    FormatNumber(result, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"sqr(%s)", oldBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_Reciprocal(StdCalcState* s) {
    if (s->hasError) return;
    double cur = StdCalc_CurrentValue(s);
    if (cur == 0.0) {
        s->hasError = TRUE;
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"Cannot divide by zero");
        return;
    }

    wchar_t oldBuf[64];
    wcscpy_s(oldBuf, ARRAYSIZE(oldBuf), s->entry);

    double result = 1.0 / cur;
    wchar_t numBuf[64];
    FormatNumber(result, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    swprintf_s(s->exprLine, ARRAYSIZE(s->exprLine), L"1/(%s)", oldBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_Sign(StdCalcState* s) {
    if (s->hasError) return;
    double cur = -StdCalc_CurrentValue(s);
    wchar_t numBuf[64];
    FormatNumber(cur, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
}

void StdCalc_Clear(StdCalcState* s) {
    s->accumulator = 0.0;
    s->lastOperand = 0.0;
    s->lastOp = 0;
    s->pendingOp = 0;
    s->startNewEntry = TRUE;
    s->hasError = FALSE;
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"0");
    s->exprLine[0] = L'\0';
}

void StdCalc_ClearEntry(StdCalcState* s) {
    s->hasError = FALSE;
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"0");
    s->startNewEntry = TRUE;
}

void StdCalc_Backspace(StdCalcState* s) {
    if (s->hasError) { StdCalc_Clear(s); return; }
    if (s->startNewEntry) return;

    size_t len = wcslen(s->entry);
    if (len <= 1 || (len == 2 && s->entry[0] == L'-')) {
        wcscpy_s(s->entry, ARRAYSIZE(s->entry), L"0");
        return;
    }
    s->entry[len - 1] = L'\0';
}

void StdCalc_MC(StdCalcState* s) {
    s->memory = 0.0;
    s->memoryHasValue = FALSE;
}

void StdCalc_MR(StdCalcState* s) {
    wchar_t numBuf[64];
    FormatNumber(s->memory, numBuf, ARRAYSIZE(numBuf));
    wcscpy_s(s->entry, ARRAYSIZE(s->entry), numBuf);
    s->startNewEntry = TRUE;
}

void StdCalc_MPlus(StdCalcState* s) {
    s->memory += StdCalc_CurrentValue(s);
    s->memoryHasValue = TRUE;
    s->startNewEntry = TRUE;
}

void StdCalc_MMinus(StdCalcState* s) {
    s->memory -= StdCalc_CurrentValue(s);
    s->memoryHasValue = TRUE;
    s->startNewEntry = TRUE;
}

void StdCalc_MS(StdCalcState* s) {
    s->memory = StdCalc_CurrentValue(s);
    s->memoryHasValue = TRUE;
    s->startNewEntry = TRUE;
}
