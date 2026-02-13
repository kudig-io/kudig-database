@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo GitBook 三级目录完整构建脚本
echo Three-Level Directory Full Build Script
echo ========================================
echo.

cd /d c:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook

echo [步骤 1/6] 生成三级目录的 SUMMARY.md...
echo [Step 1/6] Generating SUMMARY.md with 3-level structure...
powershell -NoProfile -ExecutionPolicy Bypass -File "build-scripts\generate-summary-three-level.ps1"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] SUMMARY.md 生成失败!
    echo [ERROR] SUMMARY.md generation failed!
    pause
    exit /b 1
)

echo.
echo [步骤 2/6] 验证 UTF-8 编码...
echo [Step 2/6] Verifying UTF-8 encoding...
powershell -NoProfile -Command "$file = 'src\SUMMARY.md'; if (Test-Path $file) { $bytes = [System.IO.File]::ReadAllBytes($file); $utf8 = [System.Text.Encoding]::UTF8; $content = $utf8.GetString($bytes); if ($content -match '首页' -and $content -match '核心知识域' -and $content -match '扩展领域' -and $content -match '专题内容') { Write-Host '✓ UTF-8 编码验证通过 (UTF-8 encoding verified)' -ForegroundColor Green } else { Write-Host '✗ UTF-8 编码验证失败 (UTF-8 encoding verification failed)' -ForegroundColor Red; exit 1 } }"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] UTF-8 编码验证失败!
    echo [ERROR] UTF-8 encoding verification failed!
    pause
    exit /b 1
)

echo.
echo [步骤 3/6] 清理旧的构建文件...
echo [Step 3/6] Cleaning old build files...
if exist book (
    echo 删除旧的 book 目录...
    rmdir /s /q book
)

echo.
echo [步骤 4/6] 使用 mdbook 构建静态文件...
echo [Step 4/6] Building with mdbook...
C:\Users\Allen\.local\bin\mdbook.exe build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] mdbook 构建失败!
    echo [ERROR] mdbook build failed!
    pause
    exit /b 1
)

echo.
echo [步骤 5/6] 移动到导出目录...
echo [Step 5/6] Moving to export directory...

REM 生成时间戳（简化格式）
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
    move /Y book export\!EXPORT_NAME! >nul
    echo 已移动到: export\!EXPORT_NAME!\
    echo Moved to: export\!EXPORT_NAME!\
    
    echo.
    echo [步骤 6/6] 创建 ZIP 压缩包...
    echo [Step 6/6] Creating ZIP archive...
    powershell -NoProfile -Command "Compress-Archive 'export\!EXPORT_NAME!\*' 'export\!EXPORT_NAME!.zip' -Force"
    
    echo.
    echo ========================================
    echo 构建成功! Build SUCCESS!
    echo ========================================
    echo.
    echo 导出位置 (Location): export\!EXPORT_NAME!\
    
    for %%A in ("export\!EXPORT_NAME!") do (
        set FOLDER_SIZE=0
        for /r "%%A" %%F in (*) do set /a FOLDER_SIZE+=%%~zF
    )
    
    set /a FOLDER_SIZE_MB=!FOLDER_SIZE! / 1048576
    echo 文件夹大小 (Folder size): !FOLDER_SIZE_MB! MB
    
    for %%A in ("export\!EXPORT_NAME!.zip") do (
        set /a ZIP_SIZE_MB=%%~zA / 1048576
        echo ZIP 大小 (ZIP size): !ZIP_SIZE_MB! MB (%%~zA bytes)
    )
    
    REM 统计文件数量
    for /f %%C in ('dir /s /b "export\!EXPORT_NAME!\*.html" ^| find /c ".html"') do set HTML_COUNT=%%C
    for /f %%C in ('dir /s /b "export\!EXPORT_NAME!\*.md" ^| find /c ".md"') do set MD_COUNT=%%C
    
    echo HTML 页面数 (HTML pages): !HTML_COUNT!
    echo Markdown 文件数 (MD files): !MD_COUNT!
    
    echo.
    echo 正在浏览器中打开...
    echo Opening in browser...
    start export\!EXPORT_NAME!\index.html
    
    echo.
    echo ========================================
    echo 构建完成! Build Complete!
    echo ========================================
    echo.
    echo 按任意键退出... (Press any key to exit...)
    pause >nul
) else (
    echo [错误] 构建目录未找到!
    echo [ERROR] Build directory not found!
    pause
    exit /b 1
)
