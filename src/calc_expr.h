#pragma once
#include "common.h"

typedef struct {
    wchar_t buffer[256];      /* the expression being composed / just-evaluated result */
    wchar_t historyLine[300]; /* the expression that produced the last result */
    wchar_t resultLine[64];   /* live preview while typing, or the final result */

    BOOL degMode;       /* TRUE = degrees, FALSE = radians */
    BOOL secondActive;  /* "2nd" toggle: swaps sin/cos/tan/ln/log/x^3 */
    BOOL hasError;
    BOOL justEvaluated; /* TRUE right after '=', until the next fresh entry */

    double memory;
    BOOL memoryHasValue;
} ExprCalcState;

void ExprCalc_Reset(ExprCalcState* s);

/* Appends raw display text (digits, operators, function-name-with-paren,
   constants, parens...) to the expression buffer. When continueAfterResult
   is FALSE and the state was just showing a result, the buffer is cleared
   first (mirrors pressing a digit after '='); operators pass TRUE so they
   chain off the previous result instead. */
void ExprCalc_Append(ExprCalcState* s, const wchar_t* text, BOOL continueAfterResult);

void ExprCalc_WrapReciprocal(ExprCalcState* s); /* buffer -> 1/(buffer) */
void ExprCalc_WrapSquare(ExprCalcState* s);     /* buffer -> (buffer)^2 */
void ExprCalc_WrapCube(ExprCalcState* s);       /* buffer -> (buffer)^3, or cube root if 2nd active */
void ExprCalc_ToggleSign(ExprCalcState* s);     /* wraps with -( ) or unwraps if already negated */

void ExprCalc_Backspace(ExprCalcState* s);
void ExprCalc_Clear(ExprCalcState* s);      /* C */
void ExprCalc_ClearEntry(ExprCalcState* s); /* CE: drops the trailing number being typed */
void ExprCalc_Evaluate(ExprCalcState* s);   /* = */

void ExprCalc_MC(ExprCalcState* s);
void ExprCalc_MR(ExprCalcState* s);
void ExprCalc_MPlus(ExprCalcState* s);
void ExprCalc_MMinus(ExprCalcState* s);
void ExprCalc_MS(ExprCalcState* s);

void ExprCalc_ToggleSecond(ExprCalcState* s);
void ExprCalc_ToggleDegRad(ExprCalcState* s);
