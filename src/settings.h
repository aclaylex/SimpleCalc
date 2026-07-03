#pragma once
#include "common.h"

/* Window placement (position, size, maximized state) is persisted to
   HKCU\Software\Calculator so the app reopens where it was left. */
BOOL Settings_LoadPlacement(WINDOWPLACEMENT* outPlacement);
void Settings_SavePlacement(HWND hwnd);
