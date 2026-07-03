#pragma once
#include "common.h"

typedef struct {
    double accumulator;
    double lastOperand;
    wchar_t lastOp;       /* '+','-','*','/', or 0 = none (for repeat-equals) */
    wchar_t pendingOp;    /* '+','-','*','/', or 0 = none */
    BOOL startNewEntry;
    BOOL hasError;
    double memory;
    BOOL memoryHasValue;

    wchar_t entry[64];    /* current entry text as typed */
    wchar_t exprLine[128];/* history text shown above the display */
} StdCalcState;

void StdCalc_Reset(StdCalcState* s);
void StdCalc_Digit(StdCalcState* s, wchar_t digit);
void StdCalc_Decimal(StdCalcState* s);
void StdCalc_Operator(StdCalcState* s, wchar_t op);
void StdCalc_Equals(StdCalcState* s);
void StdCalc_Percent(StdCalcState* s);
void StdCalc_Sqrt(StdCalcState* s);
void StdCalc_Square(StdCalcState* s);
void StdCalc_Reciprocal(StdCalcState* s);
void StdCalc_Sign(StdCalcState* s);
void StdCalc_Clear(StdCalcState* s);
void StdCalc_ClearEntry(StdCalcState* s);
void StdCalc_Backspace(StdCalcState* s);
void StdCalc_MC(StdCalcState* s);
void StdCalc_MR(StdCalcState* s);
void StdCalc_MPlus(StdCalcState* s);
void StdCalc_MMinus(StdCalcState* s);
void StdCalc_MS(StdCalcState* s);

double StdCalc_CurrentValue(const StdCalcState* s);
