param(
    [string]$CertificatePath = "",
    [string]$CertificatePassword = "",
    [string]$PluginvalPath = "",
    [switch]$SkipPluginval
)

$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RootDir "cmake-windows-release-build"
$ArtifactDir = Join-Path $BuildDir "FernShyPandorasBoxPlugin_artefacts\Release"
$Vst3 = Join-Path $ArtifactDir "VST3\Pandoras Box.vst3"
$Binary = Join-Path $Vst3 "Contents\x86_64-win\Pandoras Box.vst3"
$DistDir = Join-Path $RootDir "dist"

function Invoke-Checked {
    param([scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

Invoke-Checked { cmake --preset windows-release -S $RootDir }
Invoke-Checked { cmake --build --preset windows-release }
Invoke-Checked { ctest --preset windows-release }

if (-not (Test-Path -LiteralPath $Binary -PathType Leaf)) {
    throw "Missing Windows VST3 binary: $Binary"
}

$projectText = Get-Content -LiteralPath (Join-Path $RootDir "CMakeLists.txt") -Raw
$versionMatch = [regex]::Match(
    $projectText,
    'project\s*\(\s*FernShyPandorasBoxPlugin\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)',
    [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
if (-not $versionMatch.Success) {
    throw "Could not read the release version from CMakeLists.txt"
}
$Version = $versionMatch.Groups[1].Value

$signed = $false
if ($CertificatePath) {
    if (-not (Test-Path -LiteralPath $CertificatePath -PathType Leaf)) {
        throw "Code-signing certificate not found: $CertificatePath"
    }

    $signtool = Get-Command "signtool.exe" -ErrorAction SilentlyContinue
    if (-not $signtool) {
        $sdkRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
        $signtool = Get-ChildItem -Path $sdkRoot -Filter "signtool.exe" -Recurse |
            Where-Object { $_.FullName -match '\\x64\\signtool\.exe$' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
    }
    if (-not $signtool) {
        throw "signtool.exe was not found"
    }
    $signtoolPath = if ($signtool.Source) { $signtool.Source } else { $signtool.FullName }

    Invoke-Checked {
        & $signtoolPath sign `
            /fd SHA256 `
            /td SHA256 `
            /tr "http://timestamp.digicert.com" `
            /f $CertificatePath `
            /p $CertificatePassword `
            $Binary
    }
    Invoke-Checked { & $signtoolPath verify /pa /v $Binary }
    $signed = $true
}

if (-not $SkipPluginval) {
    if (-not $PluginvalPath) {
        $pluginval = Get-Command "pluginval.exe" -ErrorAction SilentlyContinue
        if ($pluginval) {
            $PluginvalPath = $pluginval.Source
        }
    }
    if (-not $PluginvalPath -or -not (Test-Path -LiteralPath $PluginvalPath -PathType Leaf)) {
        throw "pluginval.exe was not found. Pass -PluginvalPath or use -SkipPluginval."
    }
    Invoke-Checked {
        & $PluginvalPath `
            --strictness-level 10 `
            --validate-in-process `
            --validate $Vst3
    }
}

$suffix = if ($signed) { "" } else { "-unsigned" }
$Product = "PandorasBox-$Version-Windows-x64$suffix"
$OutputDir = Join-Path $DistDir $Product
$Zip = Join-Path $DistDir "$Product.zip"

Remove-Item -LiteralPath $OutputDir -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $Zip -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

Copy-Item -LiteralPath $Vst3 -Destination $OutputDir -Recurse
Copy-Item -LiteralPath (Join-Path (Split-Path $RootDir -Parent) "LICENSE.md") -Destination $OutputDir
Copy-Item -LiteralPath (Join-Path $RootDir "THIRD_PARTY_LICENSES.md") -Destination $OutputDir
Copy-Item -LiteralPath (Join-Path $RootDir "INTER_FONT_LICENSE.md") -Destination $OutputDir
Copy-Item -LiteralPath (Join-Path $RootDir "assets\README.txt") -Destination (Join-Path $OutputDir "README.txt")

$packagedBinary = Join-Path $OutputDir "Pandoras Box.vst3\Contents\x86_64-win\Pandoras Box.vst3"
$hash = (Get-FileHash -LiteralPath $packagedBinary -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  Pandoras Box.vst3/Contents/x86_64-win/Pandoras Box.vst3" |
    Set-Content -LiteralPath (Join-Path $OutputDir "SHA256SUMS.txt") -Encoding ascii

Compress-Archive -LiteralPath $OutputDir -DestinationPath $Zip -CompressionLevel Optimal

Write-Host "Windows release ready:"
Write-Host "  $Zip"
if (-not $signed) {
    Write-Warning "This package is unsigned and is not the public release artifact."
}
