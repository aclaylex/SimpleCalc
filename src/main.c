#include "app.h"
#include <stdlib.h>
#include <crtdbg.h>

/* If any *_s CRT function (swprintf_s, wcscpy_s, ...) is ever handed a
   would-be overflow, the default invalid-parameter handler aborts the
   whole process. Installing a no-op handler here converts that class of
   bug into "the affected string becomes empty" instead of a crash. */
static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function,
                                     const wchar_t* file, unsigned int line,
                                     uintptr_t reserved) {
    (void)expression;
    (void)function;
    (void)file;
    (void)line;
    (void)reserved;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;

    /* Restrict implicit DLL loading (and any future LoadLibrary calls) to
       System32, closing off DLL search-order / binary-planting hijacking
       (CWE-427) if the exe is copied into an untrusted, writable folder. */
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);

    _set_invalid_parameter_handler(InvalidParameterHandler);
    _CrtSetReportMode(_CRT_ASSERT, 0);

    if (!App_Init(hInstance, nCmdShow)) {
        return 1;
    }
    return App_Run();
}
