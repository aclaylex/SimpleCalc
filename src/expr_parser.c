#include "expr_parser.h"
#include <wctype.h>

typedef struct {
    const wchar_t* s;
    size_t pos;
    size_t len;
    BOOL degMode;
    BOOL error;
} Parser;

static double ParseExpr(Parser* p);
static double ParseTerm(Parser* p);
static double ParseUnary(Parser* p);
static double ParsePower(Parser* p);
static double ParsePostfix(Parser* p);
static double ParsePrimary(Parser* p);

static wchar_t PeekCh(Parser* p) { return p->pos < p->len ? p->s[p->pos] : 0; }

static void SkipSpaces(Parser* p) {
    while (p->pos < p->len && p->s[p->pos] == L' ') p->pos++;
}

static BOOL MatchKeyword(Parser* p, const wchar_t* kw) {
    size_t klen = wcslen(kw);
    if (p->pos + klen <= p->len && wcsncmp(p->s + p->pos, kw, klen) == 0) {
        p->pos += klen;
        return TRUE;
    }
    return FALSE;
}

static void Expect(Parser* p, wchar_t ch) {
    SkipSpaces(p);
    if (PeekCh(p) == ch) p->pos++;
    else p->error = TRUE;
}

static double CheckFinite(Parser* p, double v) {
    if (isnan(v) || isinf(v)) p->error = TRUE;
    return v;
}

static double ToRad(Parser* p, double v) { return p->degMode ? v * M_PI / 180.0 : v; }
static double FromRad(Parser* p, double v) { return p->degMode ? v * 180.0 / M_PI : v; }

static double ParseFuncArg(Parser* p) {
    double v = ParseExpr(p);
    Expect(p, L')');
    return v;
}

static double ParsePrimary(Parser* p) {
    SkipSpaces(p);
    if (p->error) return 0.0;

    wchar_t c = PeekCh(p);

    if (c == L'(') {
        p->pos++;
        double v = ParseExpr(p);
        Expect(p, L')');
        return v;
    }

    if (c == 0x03C0) { p->pos++; return M_PI; }
    if (c == L'e' && !(p->pos + 1 < p->len && iswalpha(p->s[p->pos + 1]))) {
        p->pos++;
        return M_E;
    }

    if (MatchKeyword(p, L"asin(")) return CheckFinite(p, FromRad(p, asin(ParseFuncArg(p))));
    if (MatchKeyword(p, L"acos(")) return CheckFinite(p, FromRad(p, acos(ParseFuncArg(p))));
    if (MatchKeyword(p, L"atan(")) return CheckFinite(p, FromRad(p, atan(ParseFuncArg(p))));
    if (MatchKeyword(p, L"sin(")) return CheckFinite(p, sin(ToRad(p, ParseFuncArg(p))));
    if (MatchKeyword(p, L"cos(")) return CheckFinite(p, cos(ToRad(p, ParseFuncArg(p))));
    if (MatchKeyword(p, L"tan(")) return CheckFinite(p, tan(ToRad(p, ParseFuncArg(p))));
    if (MatchKeyword(p, L"ln(")) {
        double v = ParseFuncArg(p);
        if (v <= 0.0) { p->error = TRUE; return 0.0; }
        return log(v);
    }
    if (MatchKeyword(p, L"log(")) {
        double v = ParseFuncArg(p);
        if (v <= 0.0) { p->error = TRUE; return 0.0; }
        return log10(v);
    }
    if (MatchKeyword(p, L"sqrt(")) {
        double v = ParseFuncArg(p);
        if (v < 0.0) { p->error = TRUE; return 0.0; }
        return sqrt(v);
    }

    if (iswdigit(c) || c == L'.') {
        wchar_t* end = NULL;
        double v = wcstod(p->s + p->pos, &end);
        size_t consumed = (size_t)(end - (p->s + p->pos));
        if (consumed == 0) { p->error = TRUE; return 0.0; }
        p->pos += consumed;
        return v;
    }

    p->error = TRUE;
    return 0.0;
}

static double ParsePostfix(Parser* p) {
    double v = ParsePrimary(p);
    for (;;) {
        SkipSpaces(p);
        if (PeekCh(p) == L'%') { p->pos++; v = v / 100.0; }
        else break;
    }
    return v;
}

static double ParsePower(Parser* p) {
    double base = ParsePostfix(p);
    SkipSpaces(p);
    if (PeekCh(p) == L'^') {
        p->pos++;
        double exp = ParseUnary(p);
        if (p->error) return 0.0;
        return CheckFinite(p, pow(base, exp));
    }
    return base;
}

static double ParseUnary(Parser* p) {
    SkipSpaces(p);
    wchar_t c = PeekCh(p);
    if (c == L'-' || c == 0x2212) { p->pos++; return -ParseUnary(p); }
    if (c == L'+') { p->pos++; return ParseUnary(p); }
    return ParsePower(p);
}

static double ParseTerm(Parser* p) {
    double v = ParseUnary(p);
    for (;;) {
        if (p->error) return 0.0;
        SkipSpaces(p);
        wchar_t c = PeekCh(p);
        if (c == L'*' || c == 0x00D7) {
            p->pos++;
            v *= ParseUnary(p);
        } else if (c == L'/' || c == 0x00F7) {
            p->pos++;
            double r = ParseUnary(p);
            if (p->error) return 0.0;
            if (r == 0.0) { p->error = TRUE; return 0.0; }
            v /= r;
        } else {
            break;
        }
    }
    return v;
}

static double ParseExpr(Parser* p) {
    double v = ParseTerm(p);
    for (;;) {
        if (p->error) return 0.0;
        SkipSpaces(p);
        wchar_t c = PeekCh(p);
        if (c == L'+') {
            p->pos++;
            v += ParseTerm(p);
        } else if (c == L'-' || c == 0x2212) {
            p->pos++;
            v -= ParseTerm(p);
        } else {
            break;
        }
    }
    return v;
}

BOOL EvalExpression(const wchar_t* expr, BOOL degMode, double* outResult) {
    wchar_t buf[512];
    wcsncpy_s(buf, ARRAYSIZE(buf), expr, _TRUNCATE);

    int openCount = 0;
    for (wchar_t* c = buf; *c; c++) {
        if (*c == L'(') openCount++;
        else if (*c == L')') openCount--;
    }
    size_t len = wcslen(buf);
    while (openCount > 0 && len < ARRAYSIZE(buf) - 1) {
        buf[len++] = L')';
        buf[len] = L'\0';
        openCount--;
    }

    if (len == 0) return FALSE;

    Parser p;
    p.s = buf;
    p.pos = 0;
    p.len = len;
    p.degMode = degMode;
    p.error = FALSE;

    double result = ParseExpr(&p);
    SkipSpaces(&p);

    if (p.error || p.pos != p.len || isnan(result) || isinf(result)) return FALSE;

    *outResult = result;
    return TRUE;
}
