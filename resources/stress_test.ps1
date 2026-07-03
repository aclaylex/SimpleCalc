param([int]$ProcId)

Add-Type -Namespace Native -Name Stress -MemberDefinition @"
[DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
"@

$proc = Get-Process -Id $ProcId
$hwnd = $proc.MainWindowHandle
[Native.Stress]::SetForegroundWindow($hwnd) | Out-Null

$WM_CHAR = 0x0102
$WM_LBUTTONDOWN = 0x0201
$WM_LBUTTONUP = 0x0202
$WM_SETTINGCHANGE = 0x001A
$WM_DPICHANGED = 0x02E0
$WM_GETMINMAXINFO = 0x0024
$WM_SIZE = 0x0005

function SendChar([char]$c) {
    [Native.Stress]::SendMessage($hwnd, $WM_CHAR, [IntPtr][int][char]$c, [IntPtr]::Zero) | Out-Null
}

function ClickHeaderTab([string]$which) {
    # header spans y in [0,~42*scale); standard tab is left half, scientific right half
    $rect = New-Object -TypeName PSObject
}

Write-Output "=== Test 1: flood 400 digit chars via WM_CHAR ==="
for ($i = 0; $i -lt 400; $i++) { SendChar '7' }
Start-Sleep -Milliseconds 200
$proc.Refresh()
Write-Output "after digit flood: responding=$($proc.Responding)"

Write-Output "=== Test 2: switch to Scientific, flood 400 '(' then evaluate ==="
# click scientific tab (approx: header height ~42px at 96dpi, right half of window)
$cr = New-Object -TypeName PSObject
Add-Type -Namespace Native -Name Stress2 -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out RECT lpRect);
[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hwnd, ref POINT lpPoint);
[DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, IntPtr dwExtraInfo);
public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
public struct POINT { public int X; public int Y; }
"@
Add-Type -AssemblyName System.Windows.Forms
$crect = New-Object Native.Stress2+RECT
[Native.Stress2]::GetClientRect($hwnd, [ref]$crect) | Out-Null
$tabPt = New-Object Native.Stress2+POINT -Property @{ X = [int]($crect.Right * 0.75); Y = 20 }
[Native.Stress2]::ClientToScreen($hwnd, [ref]$tabPt) | Out-Null
[System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point($tabPt.X, $tabPt.Y)
Start-Sleep -Milliseconds 80
[Native.Stress2]::mouse_event(0x0002,0,0,0,[IntPtr]::Zero)
Start-Sleep -Milliseconds 40
[Native.Stress2]::mouse_event(0x0004,0,0,0,[IntPtr]::Zero)
Start-Sleep -Milliseconds 300

for ($i = 0; $i -lt 400; $i++) { SendChar '(' }
SendChar '='
Start-Sleep -Milliseconds 300
$proc.Refresh()
Write-Output "after nested-paren flood + evaluate: responding=$($proc.Responding)"

Write-Output "=== Test 3: resize storm (tiny, huge, rapid) ==="
$sizes = @(
    @(1,1), @(0,0), @(-5,-5), @(50,50), @(5000,5000), @(300,300), @(1,900), @(900,1), @(204,485), @(480,640)
)
foreach ($s in $sizes) {
    [Native.Stress]::SetWindowPos($hwnd, [IntPtr]::Zero, 100, 100, $s[0], $s[1], 0x0004) | Out-Null
    Start-Sleep -Milliseconds 60
}
$proc.Refresh()
Write-Output "after resize storm: responding=$($proc.Responding)"

Write-Output "=== Test 4: spoofed WM_SETTINGCHANGE / WM_DPICHANGED / WM_GETMINMAXINFO with garbage pointers ==="
[Native.Stress]::SendMessage($hwnd, $WM_SETTINGCHANGE, [IntPtr]::Zero, [IntPtr]0xDEADBEEF) | Out-Null
[Native.Stress]::SendMessage($hwnd, $WM_SETTINGCHANGE, [IntPtr]::Zero, [IntPtr]1) | Out-Null
[Native.Stress]::SendMessage($hwnd, $WM_DPICHANGED, [IntPtr]0x00600060, [IntPtr]0xDEADBEEF) | Out-Null
[Native.Stress]::SendMessage($hwnd, $WM_GETMINMAXINFO, [IntPtr]::Zero, [IntPtr]0xDEADBEEF) | Out-Null
Start-Sleep -Milliseconds 200
$proc.Refresh()
Write-Output "after spoofed messages: responding=$($proc.Responding) alive=$(!$proc.HasExited)"

Write-Output "=== Test 5: rapid tab toggle spam (30x) ==="
for ($i = 0; $i -lt 30; $i++) {
    [Native.Stress]::SendMessage($hwnd, $WM_LBUTTONDOWN, [IntPtr]1, [IntPtr]((20 -shl 16) -bor 300)) | Out-Null
    [Native.Stress]::SendMessage($hwnd, $WM_LBUTTONUP, [IntPtr]0, [IntPtr]((20 -shl 16) -bor 300)) | Out-Null
}
Start-Sleep -Milliseconds 200
$proc.Refresh()
Write-Output "after tab spam: responding=$($proc.Responding)"

Write-Output "=== Final check ==="
$proc.Refresh()
Write-Output "pid=$($proc.Id) alive=$(!$proc.HasExited) responding=$($proc.Responding) workingSetMB=$([math]::Round($proc.WorkingSet64/1MB,1))"
