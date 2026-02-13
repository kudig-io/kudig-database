# generate-summary.ps1 - UTF8 Version
# Auto-generate mdBook SUMMARY.md file

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptPath = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$GitbookDir = Split-Path -Parent $ScriptPath
$ProjectRoot = Split-Path -Parent $GitbookDir
$SrcDir = Join-Path $GitbookDir "src"
$OutputFile = Join-Path $SrcDir "SUMMARY.md"

Write-Host "Generating SUMMARY.md..."

function Get-Title {
    param([string]$FilePath)
    if (Test-Path $FilePath) {
        $FirstLine = Get-Content $FilePath -First 1 -Encoding UTF8
        $title = $FirstLine -replace '^#*\s*', ''
        $title = $title -replace '\[', '\[' -replace '\]', '\]'
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
            $title = $title -replace '\[', '\[' -replace '\]', '\]'
            return $title
        }
    }
    $basename = [System.IO.Path]::GetFileNameWithoutExtension($FilePath)
    $basename = $basename -replace '\[', '\[' -replace '\]', '\]'
    return $basename
}

function Get-RelativePath {
    param([string]$FilePath)
    $RelPath = $FilePath.Replace($ProjectRoot, '').TrimStart('\', '/')
    return $RelPath -replace '\\', '/'
}

function Process-DirectoryRecursive {
    param([string]$Dir, [string]$Indent = "")
    
    $Items = @()
    $ReadmePath = Join-Path $Dir "README.md"
    
    if (Test-Path $ReadmePath) {
        $Title = Get-Title $ReadmePath
        $RelPath = Get-RelativePath $ReadmePath
        $Items += "$Indent- [$Title]($RelPath)"
    }
    
    $MdFiles = Get-ChildItem $Dir -Filter "*.md" | Where-Object { $_.Name -ne "README.md" } | Sort-Object Name
    foreach ($File in $MdFiles) {
        $Title = Get-FileTitle $File.FullName
        $RelPath = Get-RelativePath $File.FullName
        $Items += "$Indent  - [$Title]($RelPath)"
    }
    
    $SubDirs = Get-ChildItem $Dir -Directory | Sort-Object Name
    foreach ($SubDir in $SubDirs) {
        $Items += (Process-DirectoryRecursive $SubDir.FullName "$Indent  ")
    }
    
    return $Items
}

# Build content
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("# Summary")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("[首页](README.md)")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("---")
[void]$sb.AppendLine("- [核心知识域 (Domain 1-12)]()")

# Domain 1-12
for ($i = 1; $i -le 12; $i++) {
    $Dir = Get-ChildItem $ProjectRoot -Directory -Filter "domain-$i-*" | Select-Object -First 1
    if ($Dir) {
        $Items = Process-DirectoryRecursive $Dir.FullName "  "
        foreach ($item in $Items) {
            [void]$sb.AppendLine($item)
        }
    }
}

[void]$sb.AppendLine("")
[void]$sb.AppendLine("---")
[void]$sb.AppendLine("- [扩展领域 (Domain 13-33)]()")

# Domain 13-33
for ($i = 13; $i -le 33; $i++) {
    $Dir = Get-ChildItem $ProjectRoot -Directory -Filter "domain-$i-*" | Select-Object -First 1
    if ($Dir) {
        $Items = Process-DirectoryRecursive $Dir.FullName "  "
        foreach ($item in $Items) {
            [void]$sb.AppendLine($item)
        }
    }
}

[void]$sb.AppendLine("")
[void]$sb.AppendLine("---")
[void]$sb.AppendLine("- [专题内容 (Topics)]()")

# Topics
@("topic-cheat-sheet", "topic-dictionary", "topic-presentations", "topic-structural-trouble-shooting") | ForEach-Object {
    $Dir = Join-Path $ProjectRoot $_
    if (Test-Path $Dir) {
        $Items = Process-DirectoryRecursive $Dir "  "
        foreach ($item in $Items) {
            [void]$sb.AppendLine($item)
        }
    }
}

# Write UTF-8 without BOM
$Content = $sb.ToString()
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutputFile, $Content, $Utf8NoBom)

Write-Host "SUMMARY.md generated: $OutputFile"
$FileCount = ($Content | Select-String -Pattern '\.md\)' -AllMatches).Matches.Count
Write-Host "File count: $FileCount"
