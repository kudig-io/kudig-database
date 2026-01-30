# File validation script
Write-Host "=== README Link Validation ===" -ForegroundColor Green

$files = @(
    "README.md",
    "presentations/kubernetes-coredns-presentation.md",
    "presentations/kubernetes-ingress-presentation.md",
    "presentations/kubernetes-service-presentation.md", 
    "presentations/kubernetes-storage-presentation.md",
    "presentations/kubernetes-terway-presentation.md",
    "presentations/kubernetes-workload-presentation.md"
)

$validCount = 0
$totalCount = $files.Length

foreach ($file in $files) {
    if (Test-Path $file) {
        $info = Get-Item $file
        Write-Host "OK: $file (Size: $($info.Length) bytes)" -ForegroundColor Green
        $validCount++
    } else {
        Write-Host "ERROR: $file not found" -ForegroundColor Red
    }
}

Write-Host "`n=== Summary ===" -ForegroundColor Cyan
Write-Host "Total files: $totalCount"
Write-Host "Valid files: $validCount" 
Write-Host "Invalid files: $($totalCount - $validCount)"

if ($validCount -eq $totalCount) {
    Write-Host "SUCCESS: All links are valid!" -ForegroundColor Green
} else {
    Write-Host "WARNING: Some links are invalid!" -ForegroundColor Yellow
}