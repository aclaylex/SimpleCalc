#pragma once
#include "common.h"

/* Formats a double for display: trims trailing zeros, caps significant
   digits, falls back to compact scientific notation for extreme magnitudes,
   and reports "Error" for non-finite values. */
void FormatNumber(double value, wchar_t* out, size_t outCount);
