#pragma once
#include "common.h"

typedef enum {
    ACT_NONE = 0,
    ACT_DIGIT,
    ACT_DECIMAL,
    ACT_OP,
    ACT_EQUALS,
    ACT_CLEAR,
    ACT_CLEAR_ENTRY,
    ACT_BACKSPACE,
    ACT_SIGN,
    ACT_PERCENT,
    ACT_SQRT,
    ACT_SQUARE,
    ACT_RECIP,
    ACT_MC,
    ACT_MR,
    ACT_MPLUS,
    ACT_MMINUS,
    ACT_MS,
    ACT_PAREN_OPEN,
    ACT_PAREN_CLOSE,
    ACT_POW,
    ACT_CUBE,
    ACT_LN,
    ACT_EXP,
    ACT_LOG,
    ACT_POW10,
    ACT_SIN,
    ACT_COS,
    ACT_TAN,
    ACT_ASIN,
    ACT_ACOS,
    ACT_ATAN,
    ACT_SECOND,
    ACT_DEGRAD,
    ACT_PI,
    ACT_E
} ButtonAction;

typedef struct {
    int row, col;
    const wchar_t* label;
    const wchar_t* altLabel; /* shown when "2nd" is active; NULL = unaffected */
    ButtonAction action;
    ButtonAction altAction;  /* ACT_NONE = same action as primary */
    wchar_t ch;              /* digit or operator character, when relevant */
    BOOL isAccent;           /* operators / equals -> accent color */
    BOOL isFunc;             /* utility buttons -> muted gray */
} ButtonDef;

typedef struct {
    const ButtonDef* buttons;
    int count;
    int cols;
    int rows;
} Layout;

const Layout* Layout_Standard(void);
const Layout* Layout_Scientific(void);
