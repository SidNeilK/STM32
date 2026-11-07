param(
    [string]$Port = 'COM8',
    [int]$BaudRate = 115200,
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\image.jpg'),
    [int]$TimeoutSeconds = 30
)

$serial = New-Object System.IO.Ports.SerialPort $Port, $BaudRate, None, 8, One
$serial.ReadTimeout = 250
$serial.WriteTimeout = 1000

try {
    $serial.Open()
    Start-Sleep -Milliseconds 300

    [void]$serial.ReadExisting()
    $serial.Write("snap`r`n")

    $text = New-Object System.Text.StringBuilder
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $completed = $false

    while ((Get-Date) -lt $deadline) {
        try {
            $chunk = $serial.ReadExisting()
            if ($chunk.Length -gt 0) {
                [void]$text.Append($chunk)
                if ($text.ToString().Contains('JPEG sent to ESP32:')) {
                    $completed = $true
                    break
                }
            }
        }
        catch [System.IO.IOException] {
            Start-Sleep -Milliseconds 20
        }

        Start-Sleep -Milliseconds 20
    }

    if (-not $completed) {
        throw "Timed out waiting for the STM32 JPEG completion message."
    }

    $captured = $text.ToString()
    $start = $captured.IndexOf('FFD8', [System.StringComparison]::OrdinalIgnoreCase)
    $status = $captured.IndexOf('JPEG sent to ESP32:', $start, [System.StringComparison]::OrdinalIgnoreCase)

    if ($start -lt 0 -or $status -lt 0) {
        throw "The STM32 response did not contain a complete JPEG capture."
    }

    $jpegText = $captured.Substring($start, $status - $start)
    $hex = $jpegText -replace '[^0-9A-Fa-f]', ''
    $end = $hex.LastIndexOf('FFD9', [System.StringComparison]::OrdinalIgnoreCase)

    if ($end -lt 0) {
        throw "JPEG end marker FFD9 was not found."
    }

    $hex = $hex.Substring(0, $end + 4)

    if (($hex.Length % 2) -ne 0) {
        throw "The captured JPEG contains an odd number of hexadecimal characters."
    }

    $bytes = New-Object byte[] ($hex.Length / 2)
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $bytes[$i] = [Convert]::ToByte($hex.Substring($i * 2, 2), 16)
    }

    $resolvedOutputPath = [System.IO.Path]::GetFullPath($OutputPath)
    [System.IO.Directory]::CreateDirectory(
        [System.IO.Path]::GetDirectoryName($resolvedOutputPath)
    ) | Out-Null
    [System.IO.File]::WriteAllBytes($resolvedOutputPath, $bytes)

    Write-Host "Created $resolvedOutputPath ($($bytes.Length) bytes)"
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
}
