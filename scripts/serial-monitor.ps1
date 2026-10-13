param(
    [string]$Port,
    [ValidateRange(1, 3600)]
    [int]$Seconds = 60
)

$ErrorActionPreference = 'Stop'
if (-not $Port) {
    $boards = @(Get-CimInstance Win32_SerialPort |
        Where-Object { $_.PNPDeviceID -like '*VID_0483&PID_5740*' })
    if ($boards.Count -ne 1) {
        throw 'Conecte uma Black Pill executando o firmware CDC, ou informe -Port COMx.'
    }
    $Port = $boards[0].DeviceID
}

$serial = New-Object System.IO.Ports.SerialPort $Port, 115200, None, 8, One
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.DtrEnable = $true
$serial.ReadTimeout = 250
$serial.Encoding = [System.Text.Encoding]::ASCII
try {
    $serial.Open()
    Write-Host "Monitor em $Port, 115200 8N1, DTR ativo. Duracao: $Seconds segundos; Ctrl+C encerra."
    $elapsed = [Diagnostics.Stopwatch]::StartNew()
    while ($elapsed.Elapsed.TotalSeconds -lt $Seconds) {
        $received = $serial.ReadExisting()
        if ($received.Length -gt 0) {
            Write-Host -NoNewline $received
        }
        Start-Sleep -Milliseconds 50
    }
}
finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
