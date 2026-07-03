param(
    [string]$ExePath,
    [string]$OutPng
)

Add-Type -AssemblyName System.Drawing
Add-Type -Namespace Native -Name Win32 -MemberDefinition @"
[DllImport("user32.dll")]
public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
[DllImport("user32.dll")]
public static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);
[DllImport("user32.dll")]
public static extern IntPtr SetForegroundWindow(IntPtr hwnd);
public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
"@

$proc = Start-Process -FilePath $ExePath -PassThru
Start-Sleep -Milliseconds 1200

$proc.Refresh()
$hwnd = $proc.MainWindowHandle
$tries = 0
while ($hwnd -eq [IntPtr]::Zero -and $tries -lt 20) {
    Start-Sleep -Milliseconds 200
    $proc.Refresh()
    $hwnd = $proc.MainWindowHandle
    $tries++
}

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Output "FAILED: no window handle found"
    exit 1
}

[Native.Win32]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 300

$rect = New-Object Native.Win32+RECT
[Native.Win32]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$w = $rect.Right - $rect.Left
$h = $rect.Bottom - $rect.Top

$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[Native.Win32]::PrintWindow($hwnd, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$bmp.Save($OutPng, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose()
$bmp.Dispose()

Write-Output "OK pid=$($proc.Id) hwnd=$hwnd size=${w}x${h}"
