# FINAL-BUILD.ps1 - Definitive offline GitBook builder
# Run as: powershell -ExecutionPolicy Bypass -File FINAL-BUILD.ps1

param([switch]$Zip)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$ProjectRoot = Split-Path $ScriptDir
$SrcDir = Join-Path $ProjectRoot "src"
$Dist = Join-Path $ProjectRoot "dist"

Write-Host "`n=== GitBook Offline Builder ===" -ForegroundColor Cyan
Write-Host "Project: $ProjectRoot" -ForegroundColor Gray
Write-Host "Source: $SrcDir" -ForegroundColor Gray
Write-Host "Output: $Dist`n" -ForegroundColor Gray

# Find mdbook
$mdbook = if (Test-Path "$env:USERPROFILE\.local\bin\mdbook.exe") { 
    "$env:USERPROFILE\.local\bin\mdbook.exe" 
} elseif (Get-Command mdbook -EA SilentlyContinue) { 
    "mdbook" 
} else { 
    Write-Host "[ERROR] mdbook not found. Run: .\install-mdbook.ps1" -ForegroundColor Red
    exit 1 
}

Write-Host "[1/6] Cleaning old files..." -ForegroundColor Yellow
Get-ChildItem $SrcDir -ErrorAction SilentlyContinue | Where-Object { 
    $_.Name -like "domain-*" -or $_.Name -like "topic-*" 
} | Where-Object { $_.Attributes -match 'ReparsePoint' } | Remove-Item -Force

Write-Host "[2/6] Creating directory junctions..." -ForegroundColor Yellow
$linked = 0
@("domain-*", "topic-*") | ForEach-Object {
    Get-ChildItem $ProjectRoot -Directory -Filter $_ -ErrorAction SilentlyContinue | ForEach-Object {
        $link = Join-Path $SrcDir $_.Name
        if (-not (Test-Path $link)) {
            $null = cmd /c "mklink /J `"$link`" `"$($_.FullName)`"" 2>&1
            if ($?) { $linked++ }
        }
    }
}
Write-Host "  Created $linked junctions" -ForegroundColor Green

Write-Host "[3/6] Copying README..." -ForegroundColor Yellow
$readmeSrc = Join-Path (Split-Path $ProjectRoot) "README.md"
if (Test-Path $readmeSrc) {
    Copy-Item $readmeSrc (Join-Path $SrcDir "README.md") -Force
} else {
    $readmeSrc2 = Join-Path $ProjectRoot "README.md"
    if (Test-Path $readmeSrc2) {
        # README.md 已在 src 中，无需复制
        Write-Host "  README.md already in place" -ForegroundColor Gray
    }
}

Write-Host "[4/6] Generating SUMMARY.md..." -ForegroundColor Yellow
$genScript = Join-Path $ScriptDir "generate-summary-three-level.ps1"
if (Test-Path $genScript) {
    & $genScript
} else {
    Write-Host "  [WARN] generate-summary-three-level.ps1 not found, trying generate-summary-utf8-fixed.ps1" -ForegroundColor Yellow
    & (Join-Path $ScriptDir "generate-summary-utf8-fixed.ps1")
}

Write-Host "[5/6] Building with mdbook..." -ForegroundColor Yellow
# Backup & modify config
$cfg = Join-Path $ProjectRoot "book.toml"
$bak = "$cfg.backup"
Copy-Item $cfg $bak -Force

try {
    $txt = [IO.File]::ReadAllText($cfg, [Text.Encoding]::UTF8)
    $txt = $txt -replace '(?m)^site-url\s*=.*\r?$', ''
    $txt = $txt -replace 'build-dir\s*=\s*"book"', 'build-dir = "dist"'
    [IO.File]::WriteAllText($cfg, $txt, [Text.Encoding]::UTF8)
    
    # Build
    Push-Location $ProjectRoot
    $out = & $mdbook build 2>&1
    Pop-Location
    
    if ($LASTEXITCODE -ne 0) {
        $out | Write-Host -ForegroundColor Red
        throw "Build failed with code $LASTEXITCODE"
    }
    
    Write-Host "  Build completed successfully" -ForegroundColor Green
    
} finally {
    Move-Item $bak $cfg -Force
}

Write-Host "[6/6] Generating statistics..." -ForegroundColor Yellow
if (Test-Path $Dist) {
    $pages = @(Get-ChildItem "$Dist\*.html" -Recurse | Where { $_.Name -ne "print.html" }).Count
    $bytes = (Get-ChildItem $Dist -Recurse -File | Measure Length -Sum).Sum
    $mb = [math]::Round($bytes / 1MB, 2)
    
    Write-Host "`n✓ SUCCESS!" -ForegroundColor Green
    Write-Host "  Pages: $pages" -ForegroundColor Cyan
    Write-Host "  Size: $mb MB" -ForegroundColor Cyan
    Write-Host "  Location: $Dist" -ForegroundColor Cyan
    
    if ($Zip) {
        Write-Host "`nCreating ZIP archive..." -ForegroundColor Yellow
        $zipFile = Join-Path $ScriptDir "kudig-gitbook-$(Get-Date -F 'yyyyMMdd-HHmmss').zip"
        Compress-Archive "$Dist\*" $zipFile -Force
        $zipMB = [math]::Round((Get-Item $zipFile).Length/1MB, 2)
        Write-Host "✓ ZIP created: $zipMB MB" -ForegroundColor Green
        Write-Host "  File: $zipFile" -ForegroundColor Cyan
    }
    
    Write-Host "`nTo view:" -ForegroundColor Yellow
    Write-Host "  start $Dist\index.html" -ForegroundColor White
    Write-Host "`nThe 'dist' folder is now a complete offline website!" -ForegroundColor Green
    Write-Host "Copy it anywhere and open index.html to browse." -ForegroundColor Gray
    
} else {
    Write-Host "`n✗ FAILED - No output generated" -ForegroundColor Red
    exit 1
}
