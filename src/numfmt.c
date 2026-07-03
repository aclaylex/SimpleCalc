#include "numfmt.h"

static void TrimTrailingZeros(char* s) {
    /* Only trims the fractional part of plain decimal notation (no 'e'). */
    if (strchr(s, 'e') || strchr(s, 'E')) return;
    if (!strchr(s, '.')) return;

    size_t len = strlen(s);
    while (len > 0 && s[len - 1] == '0') {
        s[--len] = '\0';
    }
    if (len > 0 && s[len - 1] == '.') {
        s[--len] = '\0';
    }
}

static void PrettifyExponent(char* s, size_t sSize) {
    /* Turns "1.23e+015" / "1.23e-05" into the leaner "1.23e15" / "1.23e-5". */
    char* e = strchr(s, 'e');
    if (!e) e = strchr(s, 'E');
    if (!e) return;

    char mantissa[64];
    size_t mantLen = (size_t)(e - s);
    if (mantLen >= sizeof(mantissa)) mantLen = sizeof(mantissa) - 1;
    memcpy(mantissa, s, mantLen);
    mantissa[mantLen] = '\0';

    char* expPart = e + 1;
    int negative = 0;
    if (*expPart == '+') {
        expPart++;
    } else if (*expPart == '-') {
        negative = 1;
        expPart++;
    }
    while (*expPart == '0' && *(expPart + 1) != '\0') expPart++;

    sprintf_s(s, sSize, "%se%s%s", mantissa, negative ? "-" : "", expPart);
}

void FormatNumber(double value, wchar_t* out, size_t outCount) {
    if (outCount == 0) return;

    if (isnan(value) || isinf(value)) {
        wcsncpy_s(out, outCount, L"Error", _TRUNCATE);
        return;
    }

    /* Avoid printing "-0" for values that round to zero. */
    if (value == 0.0) value = 0.0;

    double absValue = fabs(value);
    char buf[64];

    if (absValue != 0.0 && (absValue < 1e-9 || absValue >= 1e15)) {
        sprintf_s(buf, sizeof(buf), "%.9e", value);
        /* Trim mantissa trailing zeros before the exponent marker. */
        char* e = strchr(buf, 'e');
        if (e) {
            char* p = e;
            while (p > buf && *(p - 1) == '0') p--;
            if (p > buf && *(p - 1) == '.') p--;
            memmove(p, e, strlen(e) + 1);
        }
        PrettifyExponent(buf, sizeof(buf));
    } else {
        sprintf_s(buf, sizeof(buf), "%.15g", value);
        TrimTrailingZeros(buf);
    }

    int needed = MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
    if (needed <= 0 || (size_t)needed > outCount) {
        wcsncpy_s(out, outCount, L"Error", _TRUNCATE);
        return;
    }
    MultiByteToWideChar(CP_UTF8, 0, buf, -1, out, needed);
}
