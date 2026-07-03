# SimpleCalc

A minimal, lightweight calculator for Windows 11 with Standard and Scientific
modes. Native Win32/C, no installer, no external runtime — just an
executable.

## Features

- **Standard mode**: basic arithmetic, %, √, x², 1/x, ±, memory (MC/MR/M+/M-)
- **Scientific mode**: full expression parsing with operator precedence and
  parentheses, sin/cos/tan (+ inverse via 2nd), ln/log, x^y, π, e, DEG/RAD
- Resizable window with a responsive layout; follows your Windows light/dark
  theme
- Remembers its window position, size, and maximized state between sessions
- Statically linked — depends only on stock Windows DLLs, nothing to install

## Building

Requires the MSVC toolchain (Visual Studio Build Tools, with the C++
workload). From the project root:

```
build.bat
```

Produces `build\SimpleCalc.exe`.

## Credits

App icon: [Calculator icons](https://www.flaticon.com/free-icons/calculator)
created by [Freepik](https://www.freepik.com) - [Flaticon](https://www.flaticon.com)
