# MO2 Test Automation Script
# Group 9 - CSOPESY Machine Problem 2

Write-Host "═" -ForegroundColor Cyan
Write-Host "MO2 OS EMULATOR - AUTOMATED TEST SCRIPT" -ForegroundColor Cyan
Write-Host "" -ForegroundColor Cyan
Write-Host ""

$testCases = @(
    @{Name="TC4"; Desc="Generous Memory (100% CPU expected)"; Config="config_tc4.txt"},
    @{Name="TC5"; Desc="High Paging Scenario"; Config="config_tc5.txt"},
    @{Name="TC6"; Desc="CPU Utilization Scenarios"; Config="config_tc6.txt"},
    @{Name="TC7"; Desc="Moderate Utilization with Paging"; Config="config_tc7.txt"},
    @{Name="TC8"; Desc="Deadlock Scenario"; Config="config_tc8.txt"}
)

Write-Host "Available Test Cases:" -ForegroundColor Yellow
Write-Host ""
for ($i = 0; $i -lt $testCases.Count; $i++) {
    Write-Host "  [$($i+1)] $($testCases[$i].Name) - $($testCases[$i].Desc)" -ForegroundColor White
}
Write-Host ""

$choice = Read-Host "Select test case (1-5) or 'A' for all"

if ($choice -eq 'A' -or $choice -eq 'a') {
    Write-Host ""
    Write-Host "Running ALL test cases..." -ForegroundColor Green
    Write-Host "Note: You will need to manually interact with each emulator instance" -ForegroundColor Yellow
    Write-Host ""
    
    foreach ($tc in $testCases) {
        Write-Host "" -ForegroundColor Cyan
        Write-Host "Preparing $($tc.Name): $($tc.Desc)" -ForegroundColor Cyan
        Write-Host "═" -ForegroundColor Cyan
        Copy-Item $tc.Config config.txt -Force
        Write-Host "Config copied: $($tc.Config) -> config.txt" -ForegroundColor Green
        Write-Host ""
        Write-Host "Press Enter to start $($tc.Name)..." -ForegroundColor Yellow
        Read-Host
        
        Write-Host "Starting emulator for $($tc.Name)..." -ForegroundColor Green
        Write-Host "Commands to test:"
        Write-Host "  1. initialize"
        Write-Host "  2. scheduler-test"
        Write-Host "  3. Wait appropriate time"
        Write-Host "  4. process-smi"
        Write-Host "  5. screen -ls"
        Write-Host "  6. vmstat"
        Write-Host "  7. exit"
        Write-Host ""
        
        .\mo2_emulator.exe
        
        Write-Host ""
        Write-Host "$($tc.Name) completed. Press Enter to continue..." -ForegroundColor Yellow
        Read-Host
        Write-Host ""
    }
    
    Write-Host "" -ForegroundColor Cyan
    Write-Host "ALL TEST CASES COMPLETED!" -ForegroundColor Green
    Write-Host "" -ForegroundColor Cyan
} elseif ($choice -match '^[1-5]$') {
    $selected = $testCases[[int]$choice - 1]
    
    Write-Host ""
    Write-Host "" -ForegroundColor Cyan
    Write-Host "Preparing $($selected.Name): $($selected.Desc)" -ForegroundColor Cyan
    Write-Host "" -ForegroundColor Cyan
    
    Copy-Item $selected.Config config.txt -Force
    Write-Host "Config copied: $($selected.Config) -> config.txt" -ForegroundColor Green
    Write-Host ""
    
    Write-Host "Test Instructions for $($selected.Name):" -ForegroundColor Yellow
    
    switch ($selected.Name) {
        "TC4" {
            Write-Host "  1. initialize"
            Write-Host "  2. scheduler-test"
            Write-Host "  3. Wait 2 seconds"
            Write-Host "  4. process-smi (expect ~100% CPU)"
            Write-Host "  5. screen -ls (expect mostly Running)"
            Write-Host "  6. vmstat (expect low/zero paging)"
            Write-Host "  7. exit"
        }
        "TC5" {
            Write-Host "  1. initialize"
            Write-Host "  2. scheduler-test"
            Write-Host "  3. Wait 10 seconds"
            Write-Host "  4. scheduler-stop"
            Write-Host "  5. Wait 30 seconds"
            Write-Host "  6. vmstat (expect high pages in/out)"
            Write-Host "  7. exit"
        }
        "TC6" {
            Write-Host "  1. initialize"
            Write-Host "  2. scheduler-test"
            Write-Host "  3. Periodically: screen -ls and vmstat"
            Write-Host "  4. Capture 0% and 100% CPU scenarios"
            Write-Host "  5. exit"
        }
        "TC7" {
            Write-Host "  1. initialize"
            Write-Host "  2. scheduler-test"
            Write-Host "  3. Wait 20 seconds"
            Write-Host "  4. screen -ls (repeat 5 times, 5-10s apart)"
            Write-Host "  5. scheduler-stop"
            Write-Host "  6. vmstat (expect >50% CPU, pages>0)"
            Write-Host "  7. exit"
        }
        "TC8" {
            Write-Host "  1. initialize"
            Write-Host "  2. scheduler-test"
            Write-Host "  3. Wait 5 seconds"
            Write-Host "  4. scheduler-stop"
            Write-Host "  5. Periodically: process-smi for 10s"
            Write-Host "  6. vmstat (expect 0% CPU - deadlock)"
            Write-Host "  7. exit"
        }
    }
    
    Write-Host ""
    Write-Host "Press Enter to start emulator..." -ForegroundColor Yellow
    Read-Host
    
    .\mo2_emulator.exe
    
    Write-Host ""
    Write-Host "Test completed!" -ForegroundColor Green
} else {
    Write-Host "Invalid selection. Exiting." -ForegroundColor Red
}

Write-Host ""
Write-Host "" -ForegroundColor Cyan
Write-Host "For detailed test results, check MO2_CHANGES_SUMMARY.txt" -ForegroundColor Cyan
Write-Host "" -ForegroundColor Cyan
