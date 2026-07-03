#include "layout.h"

static const ButtonDef kStandardButtons[] = {
    /* row 0 - memory */
    { 0, 0, L"MC", NULL, ACT_MC,     ACT_NONE, 0,    FALSE, TRUE },
    { 0, 1, L"MR", NULL, ACT_MR,     ACT_NONE, 0,    FALSE, TRUE },
    { 0, 2, L"M+", NULL, ACT_MPLUS,  ACT_NONE, 0,    FALSE, TRUE },
    { 0, 3, L"M-", NULL, ACT_MMINUS, ACT_NONE, 0,    FALSE, TRUE },

    /* row 1 */
    { 1, 0, L"%",       NULL, ACT_PERCENT,     ACT_NONE, L'%', FALSE, TRUE },
    { 1, 1, L"CE",      NULL, ACT_CLEAR_ENTRY, ACT_NONE, 0,    FALSE, TRUE },
    { 1, 2, L"C",       NULL, ACT_CLEAR,       ACT_NONE, 0,    FALSE, TRUE },
    { 1, 3, L"⌫",  NULL, ACT_BACKSPACE,   ACT_NONE, 0,    FALSE, TRUE },

    /* row 2 */
    { 2, 0, L"1/x",     NULL, ACT_RECIP,  ACT_NONE, 0,    FALSE, TRUE },
    { 2, 1, L"x²", NULL, ACT_SQUARE, ACT_NONE, 0,    FALSE, TRUE },
    { 2, 2, L"√",  NULL, ACT_SQRT,   ACT_NONE, 0,    FALSE, TRUE },
    { 2, 3, L"÷",  NULL, ACT_OP,     ACT_NONE, L'/', TRUE,  FALSE },

    /* row 3 */
    { 3, 0, L"7", NULL, ACT_DIGIT, ACT_NONE, L'7', FALSE, FALSE },
    { 3, 1, L"8", NULL, ACT_DIGIT, ACT_NONE, L'8', FALSE, FALSE },
    { 3, 2, L"9", NULL, ACT_DIGIT, ACT_NONE, L'9', FALSE, FALSE },
    { 3, 3, L"×", NULL, ACT_OP, ACT_NONE, L'*', TRUE, FALSE },

    /* row 4 */
    { 4, 0, L"4", NULL, ACT_DIGIT, ACT_NONE, L'4', FALSE, FALSE },
    { 4, 1, L"5", NULL, ACT_DIGIT, ACT_NONE, L'5', FALSE, FALSE },
    { 4, 2, L"6", NULL, ACT_DIGIT, ACT_NONE, L'6', FALSE, FALSE },
    { 4, 3, L"−", NULL, ACT_OP, ACT_NONE, L'-', TRUE, FALSE },

    /* row 5 */
    { 5, 0, L"1", NULL, ACT_DIGIT, ACT_NONE, L'1', FALSE, FALSE },
    { 5, 1, L"2", NULL, ACT_DIGIT, ACT_NONE, L'2', FALSE, FALSE },
    { 5, 2, L"3", NULL, ACT_DIGIT, ACT_NONE, L'3', FALSE, FALSE },
    { 5, 3, L"+", NULL, ACT_OP, ACT_NONE, L'+', TRUE, FALSE },

    /* row 6 */
    { 6, 0, L"±", NULL, ACT_SIGN,    ACT_NONE, 0,    FALSE, FALSE },
    { 6, 1, L"0",       NULL, ACT_DIGIT,   ACT_NONE, L'0', FALSE, FALSE },
    { 6, 2, L".",       NULL, ACT_DECIMAL, ACT_NONE, L'.', FALSE, FALSE },
    { 6, 3, L"=",       NULL, ACT_EQUALS,  ACT_NONE, 0,    TRUE,  FALSE },
};

static const Layout kStandardLayout = {
    kStandardButtons, ARRAYSIZE(kStandardButtons), 4, 7
};

const Layout* Layout_Standard(void) { return &kStandardLayout; }

static const ButtonDef kScientificButtons[] = {
    /* row 0 */
    { 0, 0, L"2nd", NULL, ACT_SECOND, ACT_NONE, 0, FALSE, TRUE },
    { 0, 1, L"MS",  NULL, ACT_MS,     ACT_NONE, 0, FALSE, TRUE },
    { 0, 2, L"MC",  NULL, ACT_MC,     ACT_NONE, 0, FALSE, TRUE },
    { 0, 3, L"MR",  NULL, ACT_MR,     ACT_NONE, 0, FALSE, TRUE },
    { 0, 4, L"M+",  NULL, ACT_MPLUS,  ACT_NONE, 0, FALSE, TRUE },
    { 0, 5, L"M-",  NULL, ACT_MMINUS, ACT_NONE, 0, FALSE, TRUE },

    /* row 1 */
    { 1, 0, L"(", NULL, ACT_PAREN_OPEN,  ACT_NONE, 0,    FALSE, TRUE },
    { 1, 1, L")", NULL, ACT_PAREN_CLOSE, ACT_NONE, 0,    FALSE, TRUE },
    { 1, 2, L"%", NULL, ACT_PERCENT,     ACT_NONE, L'%', FALSE, TRUE },
    { 1, 3, L"CE", NULL, ACT_CLEAR_ENTRY, ACT_NONE, 0,   FALSE, TRUE },
    { 1, 4, L"C",  NULL, ACT_CLEAR,       ACT_NONE, 0,   FALSE, TRUE },
    { 1, 5, L"⌫", NULL, ACT_BACKSPACE, ACT_NONE, 0, FALSE, TRUE },

    /* row 2 */
    { 2, 0, L"sin", L"sin⁻¹", ACT_SIN, ACT_ASIN, 0, FALSE, TRUE },
    { 2, 1, L"cos", L"cos⁻¹", ACT_COS, ACT_ACOS, 0, FALSE, TRUE },
    { 2, 2, L"1/x", NULL, ACT_RECIP,  ACT_NONE, 0, FALSE, TRUE },
    { 2, 3, L"x²", NULL, ACT_SQUARE, ACT_NONE, 0, FALSE, TRUE },
    { 2, 4, L"√", NULL, ACT_SQRT, ACT_NONE, 0, FALSE, TRUE },
    { 2, 5, L"÷", NULL, ACT_OP, ACT_NONE, L'/', TRUE, FALSE },

    /* row 3 */
    { 3, 0, L"tan", L"tan⁻¹", ACT_TAN, ACT_ATAN, 0, FALSE, TRUE },
    { 3, 1, L"DEG", NULL, ACT_DEGRAD, ACT_NONE, 0, FALSE, TRUE },
    { 3, 2, L"7", NULL, ACT_DIGIT, ACT_NONE, L'7', FALSE, FALSE },
    { 3, 3, L"8", NULL, ACT_DIGIT, ACT_NONE, L'8', FALSE, FALSE },
    { 3, 4, L"9", NULL, ACT_DIGIT, ACT_NONE, L'9', FALSE, FALSE },
    { 3, 5, L"×", NULL, ACT_OP, ACT_NONE, L'*', TRUE, FALSE },

    /* row 4 */
    { 4, 0, L"ln",  L"eˣ",  ACT_LN,  ACT_EXP,   0, FALSE, TRUE },
    { 4, 1, L"log", L"10ˣ", ACT_LOG, ACT_POW10, 0, FALSE, TRUE },
    { 4, 2, L"4", NULL, ACT_DIGIT, ACT_NONE, L'4', FALSE, FALSE },
    { 4, 3, L"5", NULL, ACT_DIGIT, ACT_NONE, L'5', FALSE, FALSE },
    { 4, 4, L"6", NULL, ACT_DIGIT, ACT_NONE, L'6', FALSE, FALSE },
    { 4, 5, L"−", NULL, ACT_OP, ACT_NONE, L'-', TRUE, FALSE },

    /* row 5 */
    { 5, 0, L"π", NULL, ACT_PI, ACT_NONE, 0, FALSE, TRUE },
    { 5, 1, L"e",       NULL, ACT_E,  ACT_NONE, 0, FALSE, TRUE },
    { 5, 2, L"1", NULL, ACT_DIGIT, ACT_NONE, L'1', FALSE, FALSE },
    { 5, 3, L"2", NULL, ACT_DIGIT, ACT_NONE, L'2', FALSE, FALSE },
    { 5, 4, L"3", NULL, ACT_DIGIT, ACT_NONE, L'3', FALSE, FALSE },
    { 5, 5, L"+", NULL, ACT_OP, ACT_NONE, L'+', TRUE, FALSE },

    /* row 6 */
    { 6, 0, L"x^y", NULL, ACT_POW, ACT_NONE, 0, FALSE, TRUE },
    { 6, 1, L"x³", L"∛x", ACT_CUBE, ACT_NONE, 0, FALSE, TRUE },
    { 6, 2, L"±", NULL, ACT_SIGN,    ACT_NONE, 0,    FALSE, FALSE },
    { 6, 3, L"0",       NULL, ACT_DIGIT,   ACT_NONE, L'0', FALSE, FALSE },
    { 6, 4, L".",       NULL, ACT_DECIMAL, ACT_NONE, L'.', FALSE, FALSE },
    { 6, 5, L"=",       NULL, ACT_EQUALS,  ACT_NONE, 0,    TRUE,  FALSE },
};

static const Layout kScientificLayout = {
    kScientificButtons, ARRAYSIZE(kScientificButtons), 6, 7
};

const Layout* Layout_Scientific(void) { return &kScientificLayout; }
