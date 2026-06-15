$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $root "build-gpp"
$compiler = "C:\msys64\ucrt64\bin\g++.exe"

if (-not (Test-Path $compiler)) {
    throw "g++ not found at $compiler"
}

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$coreSources = @(
    "src/domain/catalog/product.cpp",
    "src/domain/inventory/inventory_item.cpp",
    "src/domain/cart/cart.cpp",
    "src/domain/order/order.cpp",
    "src/domain/pricing/voucher.cpp",
    "src/domain/customer/customer.cpp",
    "src/domain/identity/account.cpp",
    "src/infrastructure/persistence/file/file_store.cpp",
    "src/infrastructure/persistence/file/file_repositories.cpp"
)

& $compiler -std=c++20 -Isrc "src/main.cpp" @coreSources -o (Join-Path $outputDir "fashion_store.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/core_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_core_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/file_persistence_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_file_persistence_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/order_consistency_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_order_consistency_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/review_return_guards_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_review_return_guards_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/return_management_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_return_management_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $compiler -std=c++20 -Isrc "tests/report_smoke.cpp" @coreSources -o (Join-Path $outputDir "fashion_store_report_smoke.exe")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Build completed in $outputDir"
