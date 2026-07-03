param(
    [int]$ProcId,
    [string]$OutPng,
    [string]$Sequence  # comma-separated tokens: "tab:scientific", "r,c" grid cells
)

Add-Type -Namespace Native -Name Win32b -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);
[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hwnd, ref POINT lpPoint);
[DllImport("user32.dll")] public static extern IntPtr SetForegroundWindow(IntPtr hwnd);
[DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hwnd);
[DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, IntPtr dwExtraInfo);
public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
public struct POINT { public int X; public int Y; }
"@
Add-Type -AssemblyName System.Drawing
Add-Type -Namespace Native -Name Win32c -MemberDefinition @"
[DllImport("user32.dll")]
public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
"@

$proc = Get-Process -Id $ProcId
$hwnd = $proc.MainWindowHandle
[Native.Win32b]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 200

$dpi = [Native.Win32b]::GetDpiForWindow($hwnd)
$scale = $dpi / 96.0

function ScalePx([double]$v) { return [int]([Math]::Round($v * $scale)) }

$PAD_OUTER = 14; $PAD_GRID = 8; $BTN_SIZE = 58; $HEADER_H = 42; $DISPLAY_H = 112

function ClientPointForCell([int]$row, [int]$col) {
    $cell = ScalePx $BTN_SIZE
    $gap = ScalePx $PAD_GRID
    $gridTop = (ScalePx $HEADER_H) + (ScalePx $DISPLAY_H) + (ScalePx $PAD_OUTER)
    $x = (ScalePx $PAD_OUTER) + $col * ($cell + $gap) + [int]($cell/2)
    $y = $gridTop + $row * ($cell + $gap) + [int]($cell/2)
    return New-Object Native.Win32b+POINT -Property @{ X = $x; Y = $y }
}

function ClientPointForTab([string]$which, [int]$clientWidth) {
    $half = [int]($clientWidth / 2)
    $y = [int]((ScalePx $HEADER_H) / 2)
    if ($which -eq "standard") { $x = [int]($half / 2) } else { $x = $half + [int]($half / 2) }
    return New-Object Native.Win32b+POINT -Property @{ X = $x; Y = $y }
}

$rect = New-Object Native.Win32b+RECT
[Native.Win32b]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
# approximate client width via window width minus small chrome margin (only used for tab x-centering)
$clientWidthGuess = $rect.Right - $rect.Left - 16

function ClickScreenPoint([Native.Win32b+POINT]$clientPt) {
    $pt = New-Object Native.Win32b+POINT -Property @{ X = $clientPt.X; Y = $clientPt.Y }
    [Native.Win32b]::ClientToScreen($hwnd, [ref]$pt) | Out-Null
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point($pt.X, $pt.Y)
    Start-Sleep -Milliseconds 60
    [Native.Win32b]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero) # left down
    Start-Sleep -Milliseconds 40
    [Native.Win32b]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero) # left up
    Start-Sleep -Milliseconds 120
}

Add-Type -AssemblyName System.Windows.Forms

foreach ($tok in $Sequence.Split(',')) {
    $tok = $tok.Trim()
    if ($tok -eq "") { continue }
    if ($tok -eq "tab:standard" -or $tok -eq "tab:scientific") {
        $which = $tok.Split(':')[1]
        $p = ClientPointForTab $which $clientWidthGuess
        ClickScreenPoint $p
    } else {
        $parts = $tok.Split(':')
        $row = [int]$parts[0]; $col = [int]$parts[1]
        $p = ClientPointForCell $row $col
        ClickScreenPoint $p
    }
}

Start-Sleep -Milliseconds 200
$rect2 = New-Object Native.Win32b+RECT
[Native.Win32b]::GetWindowRect($hwnd, [ref]$rect2) | Out-Null
$w = $rect2.Right - $rect2.Left
$h = $rect2.Bottom - $rect2.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[Native.Win32c]::PrintWindow($hwnd, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$bmp.Save($OutPng, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "done"
