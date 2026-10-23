param(
    [ValidateSet("x64", "ARM64")]
    [string]$Architecture = "x64",
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build-windows"
$distRoot = Join-Path $root "dist"
$packageDir = Join-Path $distRoot "windows"
$version = "1.0.0"

Push-Location $root

try {
    Write-Host "Configuring Cerberus for Windows $Architecture..."
    cmake -S . -B $buildDir `
        -A $Architecture `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DCERBERUS_BUILD_SCREENSHOT=OFF `
        -DCERBERUS_BUILD_HOST_TEST=ON

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed."
    }

    Write-Host "Building..."
    cmake --build $buildDir --config Release --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed."
    }

    $testExe = Join-Path $buildDir "CerberusTests_artefacts\Release\CerberusTests.exe"
    $hostTest = Join-Path $buildDir "CerberusHostSmokeTest_artefacts\Release\CerberusHostSmokeTest.exe"
    $vst3 = Join-Path $buildDir "Cerberus_artefacts\Release\VST3\Cerberus.vst3"
    $standalone = Join-Path $buildDir "Cerberus_artefacts\Release\Standalone\Cerberus.exe"

    if (-not $SkipTests) {
        Write-Host "Running DSP tests..."
        & $testExe
        if ($LASTEXITCODE -ne 0) {
            throw "DSP tests failed."
        }

        Write-Host "Running VST3 host test..."
        & $hostTest $vst3
        if ($LASTEXITCODE -ne 0) {
            throw "VST3 host test failed."
        }
    }

    if (-not (Test-Path $vst3)) {
        throw "VST3 bundle was not produced at $vst3"
    }

    if (-not (Test-Path $standalone)) {
        throw "Standalone executable was not produced at $standalone"
    }

    if (Test-Path $packageDir) {
        Remove-Item -Recurse -Force $packageDir
    }

    New-Item -ItemType Directory -Force $packageDir | Out-Null
    Copy-Item -Recurse -Force $vst3 $packageDir
    Copy-Item -Force $standalone $packageDir
    Copy-Item -Force "README.md", "Resources\Lato-OFL.txt" $packageDir

    $zipName = "Cerberus-Windows-$Architecture-v$version.zip"
    $zipPath = Join-Path $distRoot $zipName
    Compress-Archive -Path "$packageDir\*" -DestinationPath $zipPath -Force

    $hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLower()
    "$hash  $zipName" | Out-File -Encoding ascii (Join-Path $distRoot "SHA256SUMS-Windows.txt")

    Write-Host ""
    Write-Host "Distribution ready:"
    Write-Host "  $zipPath"
    Write-Host ""
    Write-Host "Install the VST3 by copying Cerberus.vst3 to:"
    Write-Host "  $env:CommonProgramFiles\VST3"
    Write-Host "Or run the standalone executable directly."
}
finally {
    Pop-Location
}
