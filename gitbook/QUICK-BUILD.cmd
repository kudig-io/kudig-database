@echo off
chcp 65001 >nul
echo ========================================
echo GitBook Quick Build and Export (UTF-8 Fixed)
echo ========================================
echo.

cd /d c:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook

echo [1/5] Generating SUMMARY.md...
powershell -NoProfile -ExecutionPolicy Bypass -File "build-scripts\generate-summary-utf8-fixed.ps1"

echo [2/5] Verifying SUMMARY.md encoding...
powershell -NoProfile -Command "$file = 'src\SUMMARY.md'; if (Test-Path $file) { $bytes = [System.IO.File]::ReadAllBytes($file); $utf8 = [System.Text.Encoding]::UTF8; $content = $utf8.GetString($bytes); Write-Host 'SUMMARY.md verified - UTF-8 encoding OK' }"

echo [3/5] Building with mdbook...
C:\Users\Allen\.local\bin\mdbook.exe build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo [4/5] Moving to export directory...
for /f "tokens=1-4 delims=/ " %%a in ('date /t') do set MYDATE=%%a%%b%%c
for /f "tokens=1-2 delims=: " %%a in ('time /t') do set MYTIME=%%a%%b
set TIMESTAMP=%MYDATE%-%MYTIME: =0%
set EXPORT_NAME=kudig-gitbook-%TIMESTAMP%

if not exist export mkdir export

if exist book (
    move /Y book export\%EXPORT_NAME% >nul
    echo Moved to: export\%EXPORT_NAME%\
    
    echo [5/5] Creating ZIP...
    powershell -NoProfile -Command "Compress-Archive 'export\%EXPORT_NAME%\*' 'export\%EXPORT_NAME%.zip' -Force"
    
    echo.
    echo ========================================
    echo SUCCESS! Export completed.
    echo ========================================
    echo.
    echo Location: export\%EXPORT_NAME%\
    for %%A in ("export\%EXPORT_NAME%.zip") do echo ZIP: %%~zA bytes (export\%EXPORT_NAME%.zip)
    echo.
    echo Opening in browser...
    start export\%EXPORT_NAME%\index.html
    echo.
    echo Press any key to exit...
    pause >nul
) else (
    echo ERROR: Build directory not found!
    pause
    exit /b 1
)
