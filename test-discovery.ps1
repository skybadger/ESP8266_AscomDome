#requires -Version 5.1
<#
.SYNOPSIS
Discover Alpaca servers, list managed devices, and read dome settings using curl.
.EXAMPLE
.\test-discovery.ps1
.EXAMPLE
.\test-discovery.ps1 -DiscoveryAddress 192.168.1.255 -DeviceName ESPdom01
.EXAMPLE
.\test-discovery.ps1 -DiscoveryAddress 192.168.1.89
#>
[CmdletBinding()]
param(
    [string]$DiscoveryAddress = '255.255.255.255',
    [ValidateRange(1,65535)][int]$DiscoveryPort = 32227,
    [ValidateRange(1,60)][int]$DiscoverySeconds = 4,
    [ValidateRange(1,120)][int]$HttpTimeoutSeconds = 15,
    [ValidateRange(1,2147483647)][int]$ClientID = (Get-Random -Minimum 10000 -Maximum 2147483647),
    [string]$DeviceName,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot ('logs\discovery-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))
)

$ErrorActionPreference = 'Stop'
$curl = (Get-Command curl.exe -ErrorAction Stop).Source
$null = New-Item -ItemType Directory -Path $OutputDirectory -Force
$script:transaction = 0
$script:failures = 0

function Invoke-DomeHttp
{
    param([string]$BaseUrl, [string]$Path, [string]$Method = 'GET',
        [string]$Form = '', [switch]$Raw)
    $script:transaction++
    $ids = "ClientID=$ClientID&ClientTransactionID=$script:transaction"
    $url = "$BaseUrl$Path"
    $fileName = '{0:D4}-{1}-{2}.txt' -f $script:transaction, $Method, ($Path.Trim('/') -replace '/', '-')
    $file = Join-Path $OutputDirectory $fileName
    $arguments = @('--silent', '--show-error', '--fail', '--noproxy', '*',
                    '--connect-timeout', '5', '--max-time', "$HttpTimeoutSeconds",
                    '--output', $file, '--request', $Method)
    if ($Method -eq 'GET')
    {
        $url += "?$ids"
    }
    else
    {
        $arguments += @('--data', "$ids&$Form")
    }
    Write-Host "$Method $url $Form"
    & $curl @arguments $url
    if ($LASTEXITCODE -ne 0)
    {
        throw "curl failed ($LASTEXITCODE): $Method $url"
    }
    $body = Get-Content -LiteralPath $file -Raw
    if ($Raw)
    {
        Write-Host "Saved $file"; return
    }
    $json = $body | ConvertFrom-Json
    if ($null -eq $json.ErrorNumber)
    {
        throw "Missing Alpaca ErrorNumber: $url (saved in $file)"
    }
    if ($json.ErrorNumber -ne 0)
    {
        throw "Alpaca error $($json.ErrorNumber): $($json.ErrorMessage) [$url]"
    }
    Write-Host ($json | ConvertTo-Json -Depth 12)
    return $json
}

# curl does not send UDP; use a .NET socket on an ephemeral reply port.
$udp = New-Object System.Net.Sockets.UdpClient
$servers = @{}
try
{
    $udp.EnableBroadcast = $true
    $udp.Client.Bind((New-Object System.Net.IPEndPoint ([System.Net.IPAddress]::Any, 0)))
    $packet = [System.Text.Encoding]::ASCII.GetBytes('alpacadiscovery1')
    Write-Host "UDP discovery to ${DiscoveryAddress}:$DiscoveryPort; ClientID=$ClientID"
    for ($attempt = 0; $attempt -lt 3; $attempt++)
    {
        $null = $udp.Send($packet, $packet.Length, $DiscoveryAddress, $DiscoveryPort)
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        while ($timer.Elapsed.TotalSeconds -lt $DiscoverySeconds)
        {
            $udp.Client.ReceiveTimeout = [Math]::Max(1, [int](1000 * $DiscoverySeconds - $timer.ElapsedMilliseconds))
            $sender = New-Object System.Net.IPEndPoint ([System.Net.IPAddress]::Any, 0)
            try
            {
                $bytes = $udp.Receive([ref]$sender)
            }
            catch [System.Net.Sockets.SocketException]
            {
                if ($_.Exception.SocketErrorCode -eq 'TimedOut')
                {
                    break
                }
                throw
            }
            try
            {
                $reply = [System.Text.Encoding]::UTF8.GetString($bytes) | ConvertFrom-Json
                $port = 0
                if (-not [int]::TryParse([string]$reply.AlpacaPort, [ref]$port) -or $port -lt 1 -or $port -gt 65535)
                {
                    throw 'Invalid AlpacaPort'
                }
                $base = "http://$($sender.Address):$port"
                $servers[$base] = $true
                Write-Host "Discovered $base"
            }
            catch
            {
                Write-Warning "Ignoring discovery reply from ${sender}: $_"
            }
        }
    }
}
finally
{
    $udp.Close()
}
if ($servers.Count -eq 0)
{
    throw 'No Alpaca servers found. Try -DiscoveryAddress with the subnet broadcast or dome IP; check UDP 32227 and firewall access.'
}

$domes = @()
foreach ($base in ($servers.Keys | Sort-Object))
{
    try
    {
        $versions = Invoke-DomeHttp $base '/management/apiversions'
        if (1 -notin @($versions.Value))
        {
            throw "Server does not advertise API v1: $base"
        }
        $null = Invoke-DomeHttp $base '/management/v1/description'
        $devices = Invoke-DomeHttp $base '/management/v1/configureddevices'
        foreach ($device in $devices.Value)
        {
            if ($device.DeviceType -ieq 'Dome' -and (!$DeviceName -or $device.DeviceName -ieq $DeviceName))
            {
                $number = 0
                if (-not [int]::TryParse([string]$device.DeviceNumber, [ref]$number) -or $number -lt 0)
                {
                    throw "Invalid dome DeviceNumber from $base"
                }
                $domes += [pscustomobject]@{ Base = $base; Number = $number; Name = $device.DeviceName }
            }
        }
    }
    catch
    {
        $script:failures++; Write-Warning $_
    }
}
if ($domes.Count -eq 0)
{
    throw 'No matching Dome devices returned by management. See saved responses.'
}

foreach ($dome in $domes)
{
    $path = "/api/v1/dome/$($dome.Number)"
    $connectionAttempted = $false
    try
    {
        Write-Host "Testing $($dome.Name) at $($dome.Base)$path"
        # Set before the PUT so a lost response still triggers cleanup.
        $connectionAttempted = $true
        $null = Invoke-DomeHttp $dome.Base "$path/connected" PUT 'Connected=true'
        $state = Invoke-DomeHttp $dome.Base "$path/connected"
        if ($state.Value -ne $true)
        {
            throw 'Connected GET did not return true'
        }
        foreach ($property in @('name','description','driverinfo','driverversion','interfaceversion',
            'supportedactions','canfindhome','canpark','cansetaltitude','cansetazimuth',
            'cansetpark','cansetshutter','canslave','cansyncazimuth','altitude','azimuth',
            'athome','atpark','shutterstatus','slaved','slewing'))
        {
            try
            {
                $null = Invoke-DomeHttp $dome.Base "$path/$property"
            }
            catch
            {
                $script:failures++; Write-Warning $_
            }
        }
        # Save both the shared management page and the per-driver configuration.
        # The legacy UI serves the same form at both routes.
        Invoke-DomeHttp $dome.Base '/setup' -Raw
        Invoke-DomeHttp $dome.Base "/setup/v1/dome/$($dome.Number)/setup" -Raw
    }
    catch
    {
        $script:failures++; Write-Warning $_
    }
    finally
    {
        if ($connectionAttempted)
        {
            try
            {
                $null = Invoke-DomeHttp $dome.Base "$path/connected" PUT 'Connected=false'
                $state = Invoke-DomeHttp $dome.Base "$path/connected"
                if ($state.Value -ne $false)
                {
                    throw 'Connected GET did not return false after disconnect'
                }
            }
            catch
            {
                $script:failures++; Write-Warning "Disconnect failed: $_"
            }
        }
    }
}
Write-Host "Responses saved to $OutputDirectory"
if ($script:failures)
{
    throw "$script:failures test request(s) failed. See warnings and saved responses."
}
Write-Host 'PASS: discovery, management, dome properties, setup and disconnect completed.'
