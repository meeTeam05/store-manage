$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildScript = Join-Path $PSScriptRoot "build-gpp.ps1"
$smokeExe = Join-Path $root "build-gpp/fashion_store_core_smoke.exe"
$persistenceSmokeExe = Join-Path $root "build-gpp/fashion_store_file_persistence_smoke.exe"
$orderConsistencySmokeExe = Join-Path $root "build-gpp/fashion_store_order_consistency_smoke.exe"
$reviewReturnGuardsSmokeExe = Join-Path $root "build-gpp/fashion_store_review_return_guards_smoke.exe"
$returnManagementSmokeExe = Join-Path $root "build-gpp/fashion_store_return_management_smoke.exe"
$reportSmokeExe = Join-Path $root "build-gpp/fashion_store_report_smoke.exe"

& powershell -ExecutionPolicy Bypass -File $buildScript
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $smokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $persistenceSmokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $orderConsistencySmokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $reviewReturnGuardsSmokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $returnManagementSmokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $reportSmokeExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Smoke tests passed"
