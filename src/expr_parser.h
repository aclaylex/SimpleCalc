#pragma once
#include "common.h"

/* Parses and evaluates a scientific-calculator expression (supports
   + - * / ^ % parentheses, sin/cos/tan/asin/acos/atan/ln/log/sqrt, and the
   constants pi/e). Automatically closes unmatched '(' before parsing.
   Returns FALSE on syntax or math-domain errors. */
BOOL EvalExpression(const wchar_t* expr, BOOL degMode, double* outResult);
