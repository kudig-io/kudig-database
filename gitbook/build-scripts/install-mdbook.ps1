# install-mdbook.ps1
$ErrorActionPreference = "Stop"

Write-Host "[INFO] Installing mdbook..." -ForegroundColor Green

$version = "v0.4.40"
$url = "https://github.com/rust-lang/mdBook/releases/download/$version/mdbook-$version-x86_64-pc-windows-msvc.zip"
$tempDir = Join-Path $env:TEMP "mdbook-dl"
$userBin = Join-Path $env:USERPROFILE ".local\bin"

Write-Host "[INFO] Download URL: $url"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

$zipFile = Join-Path $tempDir "mdbook.zip"
Write-Host "[INFO] Downloading..."
try {
    Invoke-WebRequest -Uri $url -OutFile $zipFile -UseBasicParsing
    Write-Host "[OK] Download complete" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Download failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Extracting..."
Expand-Archive -Path $zipFile -DestinationPath $tempDir -Force

Write-Host "[INFO] Installing to: $userBin"
New-Item -ItemType Directory -Force -Path $userBin | Out-Null

$mdbookExe = Join-Path $tempDir "mdbook.exe"
$targetExe = Join-Path $userBin "mdbook.exe"
Copy-Item $mdbookExe -Destination $targetExe -Force

Write-Host "[INFO] Adding to PATH..."
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($currentPath -notlike "*$userBin*") {
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$userBin", "User")
    Write-Host "[OK] PATH updated" -ForegroundColor Green
}

$env:Path = "$env:Path;$userBin"

Write-Host "`n[INFO] Verifying installation..."
try {
    $versionOutput = & $targetExe --version
    Write-Host "[OK] $versionOutput" -ForegroundColor Green
} catch {
    Write-Host "[WARN] Please restart terminal" -ForegroundColor Yellow
}

Write-Host "`n[INFO] Cleaning up..."
Remove-Item -Recurse -Force $tempDir

Write-Host "`n[SUCCESS] mdbook installed to: $targetExe" -ForegroundColor Green
Write-Host "[INFO] Please restart PowerShell to use mdbook" -ForegroundColor Cyan
