# generate-summary-three-level.ps1
# Auto-generate mdBook SUMMARY.md with 3-level structure and guaranteed UTF-8 encoding

$ErrorActionPreference = "Stop"

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptPath = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$GitbookDir = Split-Path -Parent $ScriptPath
$ProjectRoot = Split-Path -Parent $GitbookDir
$SrcDir = Join-Path $GitbookDir "src"
$OutputFile = Join-Path $SrcDir "SUMMARY.md"

Write-Host "Generating SUMMARY.md with 3-level structure..." -ForegroundColor Cyan

function Get-Title {
    param([string]$FilePath)
    if (Test-Path $FilePath) {
        $FirstLine = Get-Content $FilePath -First 1 -Encoding UTF8
        $title = $FirstLine -replace '^#*\s*', ''
        return $title
    }
    return Split-Path (Split-Path $FilePath -Parent) -Leaf
}

function Get-FileTitle {
    param([string]$FilePath)
    if (Test-Path $FilePath) {
        $FirstLine = Get-Content $FilePath -First 1 -Encoding UTF8
        if ($FirstLine -match '^#') {
            $title = $FirstLine -replace '^#*\s*', ''
            return $title
        }
    }
    return [System.IO.Path]::GetFileNameWithoutExtension($FilePath)
}

function Get-RelativePath {
    param([string]$FilePath)
    $RelPath = $FilePath.Replace($ProjectRoot, '').TrimStart('\', '/')
    return $RelPath -replace '\\', '/'
}

function Process-DirectoryRecursive {
    param(
        [string]$Dir, 
        [string]$Indent = "",
        [int]$MaxDepth = 3,
        [int]$CurrentDepth = 0
    )
    
    $Items = @()
    
    if ($CurrentDepth -ge $MaxDepth) {
        return $Items
    }
    
    $ReadmePath = Join-Path $Dir "README.md"
    
    if (Test-Path $ReadmePath) {
        $Title = Get-Title $ReadmePath
        $RelPath = Get-RelativePath $ReadmePath
        $Items += "$Indent- [$Title]($RelPath)"
    }
    
    $MdFiles = Get-ChildItem $Dir -Filter "*.md" -ErrorAction SilentlyContinue | 
               Where-Object { $_.Name -ne "README.md" } | 
               Sort-Object Name
    
    foreach ($File in $MdFiles) {
        $Title = Get-FileTitle $File.FullName
        $RelPath = Get-RelativePath $File.FullName
        $Items += "$Indent  - [$Title]($RelPath)"
    }
    
    $SubDirs = Get-ChildItem $Dir -Directory -ErrorAction SilentlyContinue | Sort-Object Name
    
    foreach ($SubDir in $SubDirs) {
        $SubItems = Process-DirectoryRecursive -Dir $SubDir.FullName -Indent "$Indent    " -MaxDepth $MaxDepth -CurrentDepth ($CurrentDepth + 1)
        $Items += $SubItems
    }
    
    return $Items
}

$lines = New-Object System.Collections.ArrayList

$null = $lines.Add("# Summary")
$null = $lines.Add("")
$null = $lines.Add("[" + [char]0x9996 + [char]0x9875 + "](README.md)")
$null = $lines.Add("")
$null = $lines.Add("---")
$null = $lines.Add("- [" + [char]0x6838 + [char]0x5FC3 + [char]0x77E5 + [char]0x8BC6 + [char]0x57DF + " (Domain 1-12)]()")

Write-Host "Processing Domain 1-12..." -ForegroundColor Yellow

for ($i = 1; $i -le 12; $i++) {
    $Dir = Get-ChildItem $ProjectRoot -Directory -Filter "domain-$i-*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Dir) {
        Write-Host "  Processing $($Dir.Name)..." -ForegroundColor Gray
        $Items = Process-DirectoryRecursive -Dir $Dir.FullName -Indent "  " -MaxDepth 3
        foreach ($item in $Items) {
            $null = $lines.Add($item)
        }
    }
}

$null = $lines.Add("")
$null = $lines.Add("---")
$null = $lines.Add("- [" + [char]0x6269 + [char]0x5C55 + [char]0x9886 + [char]0x57DF + " (Domain 13-33)]()")

Write-Host "Processing Domain 13-33..." -ForegroundColor Yellow

for ($i = 13; $i -le 33; $i++) {
    $Dir = Get-ChildItem $ProjectRoot -Directory -Filter "domain-$i-*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Dir) {
        Write-Host "  Processing $($Dir.Name)..." -ForegroundColor Gray
        $Items = Process-DirectoryRecursive -Dir $Dir.FullName -Indent "  " -MaxDepth 3
        foreach ($item in $Items) {
            $null = $lines.Add($item)
        }
    }
}

$null = $lines.Add("")
$null = $lines.Add("---")
$null = $lines.Add("- [" + [char]0x4E13 + [char]0x9898 + [char]0x5185 + [char]0x5BB9 + " (Topics)]()")

Write-Host "Processing Topics..." -ForegroundColor Yellow

@("topic-cheat-sheet", "topic-dictionary", "topic-presentations", "topic-structural-trouble-shooting") | ForEach-Object {
    $Dir = Join-Path $ProjectRoot $_
    if (Test-Path $Dir) {
        Write-Host "  Processing $_..." -ForegroundColor Gray
        $Items = Process-DirectoryRecursive -Dir $Dir -Indent "  " -MaxDepth 3
        foreach ($item in $Items) {
            $null = $lines.Add($item)
        }
    }
}

$Content = $lines -join "`n"

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutputFile, $Content, $Utf8NoBom)

Write-Host ""
Write-Host "SUMMARY.md generated successfully" -ForegroundColor Green

$FileCount = ($Content -split '\.md\)').Count - 1
Write-Host "Total file links: $FileCount" -ForegroundColor Cyan
