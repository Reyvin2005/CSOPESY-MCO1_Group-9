# Automated MO2 Test Script
Write-Host "
" -ForegroundColor Cyan
Write-Host "TEST CASE 4: Generous Memory Scenario" -ForegroundColor Cyan
Write-Host "" -ForegroundColor Cyan

# Copy TC4 config
Copy-Item config_tc4.txt config.txt -Force
Write-Host "Config: 4 CPUs, 32768 KB memory, 32 KB frames" -ForegroundColor Yellow
Get-Content config.txt | Write-Host -ForegroundColor Gray

Write-Host "
Expected: 100% CPU utilization, minimal paging
" -ForegroundColor Green
