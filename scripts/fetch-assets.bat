@echo off
setlocal DisableDelayedExpansion
if "%~1"=="--help" goto help
if not "%~2"=="" goto help
set "binary=%APACHE_EXECUTABLE%"
if not defined binary if exist "%~dp0..\apache.exe" set "binary=%~dp0..\apache.exe"
if not defined binary if exist "%~dp0..\build\apache.exe" set "binary=%~dp0..\build\apache.exe"
if not defined binary (
  echo Build Apache first, or set APACHE_EXECUTABLE to its executable.
  exit /b 1
)
set "destination=%~1"
if not defined destination for /f "delims=" %%D in ('"%binary%" --asset-directory') do set "destination=%%D"
if not defined destination exit /b 1
"%binary%" --check-assets --data-dir "%destination%" >nul 2>&1
if not errorlevel 1 (
  echo Game resources are already installed in %destination%
  exit /b 0
)
set "download=%TEMP%\apache-cd-%RANDOM%-%RANDOM%.bin"
if exist "%download%" exit /b 1
echo Downloading the Apache Longbow reference CD (358 MiB)...
curl.exe --user-agent "Lost Engines Apache CPP" --fail --location --retry 3 --retry-all-errors --connect-timeout 30 --output "%download%" "https://archive.org/download/redump-id-91728/Apache%%20Longbow%%20%%28Europe%%29%%20%%28Rerelease%%29%%20%%2820001017%%29.bin"
if errorlevel 1 goto failed
set "matched="
for /f "delims=" %%H in ('certutil.exe -hashfile "%download%" SHA256 2^>nul') do if /I "%%H"=="1a21c143c52c4cbad446ecaf4ebb5429307736b4061f20c3abe13128ee8e374a" set "matched=1"
if not defined matched (
  echo The CD checksum does not match. Please retry.
  goto failed
)
"%binary%" --install-cd "%download%" --data-dir "%destination%"
if errorlevel 1 goto failed
"%binary%" --check-assets --data-dir "%destination%"
if errorlevel 1 goto failed
del "%download%"
echo Ready.
exit /b 0
:failed
if exist "%download%" del "%download%"
exit /b 1
:help
echo Usage: %~nx0 [destination]
echo APACHE_EXECUTABLE selects the built Apache executable when necessary.
exit /b 0
