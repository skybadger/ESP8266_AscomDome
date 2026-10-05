$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$source = Get-Content ASCOM_DomeCmds.h -Raw
$start = $source.IndexOf('static int domeLockDetectedCount')
$end = $source.IndexOf('//Function to issue commands to shutter')
if ($start -lt 0 -or $end -le $start) { throw 'Cannot locate production dome functions' }
Set-Content build/dome-recovery-under-test.h $source.Substring($start, $end - $start)
$commands = @'
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /Fo:build\test-dome-recovery.obj /Fe:build\test-dome-recovery.exe tests\test-dome-recovery.cpp
if errorlevel 1 exit /b 1
build\test-dome-recovery.exe
'@
Set-Content build/test-dome-recovery.bat $commands
cmd /c build\test-dome-recovery.bat
if ($LASTEXITCODE -ne 0) { throw "Recovery regression failed: $LASTEXITCODE" }
