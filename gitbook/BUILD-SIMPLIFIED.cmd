@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo GitBook 三级目录完整构建脚本
echo Three-Level Directory Full Build Script
echo ========================================
echo.

REM 确保在gitbook目录中
cd /d %~dp0

echo [步骤 1/5] 生成三级目录的 SUMMARY.md...
powershell -NoProfile -ExecutionPolicy Bypass -File "build-scripts\generate-summary-three-level.ps1"

if %ERRORLEVEL% NEQ 0 (
    echo [错误] SUMMARY.md 生成失败!
    pause
    exit /b 1
)

echo.
echo [步骤 2/5] 清理旧的构建文件...
if exist book (
    echo 删除旧的 book 目录...
    rmdir /s /q book
)

echo.
echo [步骤 3/5] 使用 mdbook 构建静态文件...
C:\Users\Allen\.local\bin\mdbook.exe build

if %ERRORLEVEL% NEQ 0 (
    echo [错误] mdbook 构建失败!
    pause
    exit /b 1
)

echo.
echo [步骤 4/5] 移动到导出目录...

REM 生成时间戳
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (
    set MM=%%a
    set DD=%%b
    set YY=%%c
)
for /f "tokens=1-2 delims=: " %%a in ('time /t') do set MYTIME=%%a%%b
set MYTIME=!MYTIME: =0!

set TIMESTAMP=20!YY!!MM!!DD!-!MYTIME!
set EXPORT_NAME=kudig-gitbook-!TIMESTAMP!

if not exist export mkdir export

if exist book (
    echo 移动 book 到 export\!EXPORT_NAME!\
    move /Y book export\!EXPORT_NAME! >nul
    
    echo.
    echo [步骤 5/5] 创建 ZIP 压缩包...
    powershell -NoProfile -Command "Compress-Archive 'export\!EXPORT_NAME!\*' 'export\!EXPORT_NAME!.zip' -Force"
    
    echo.
    echo ========================================
    echo 构建成功! Build SUCCESS!
    echo ========================================
    echo.
    echo 导出位置: export\!EXPORT_NAME!\
    
    for %%A in ("export\!EXPORT_NAME!.zip") do (
        set /a ZIP_SIZE_MB=%%~zA / 1048576
        echo ZIP 大小: !ZIP_SIZE_MB! MB
    )
    
    echo.
    echo 正在浏览器中打开...
    start export\!EXPORT_NAME!\index.html
    
    echo.
    echo 按任意键退出...
    pause >nul
) else (
    echo [错误] 构建目录未找到!
    pause
    exit /b 1
)
