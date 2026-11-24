<#
.SYNOPSIS
    Copies the Twitch SDK Unreal plugin into this sample project.

.DESCRIPTION
    Copies the plugin source from R66_Unreal\Plugins\TwitchSDK to Plugins\TwitchSDK and bundles
    the C++ core library inside it (Plugins\TwitchSDK\core), the same layout that
    unreal-build\build.bat stages for packaging. The plugin's TwitchSDK.Build.cs looks for the core
    by walking up from its own folder, so the bundled copy makes the sample self-contained.

    Unreal compiles project plugins together with the project, so the next build picks up the
    copied sources. Re-run this script after changing the plugin or the core library.
#>
param(
    [string]$PluginSource = (Join-Path $PSScriptRoot "..\R66_Unreal\Plugins\TwitchSDK"),
    [string]$CoreSource = (Join-Path $PSScriptRoot "..\core")
)

$ErrorActionPreference = "Stop"

foreach ($required in @($PluginSource, (Join-Path $PluginSource "TwitchSDK.uplugin"), (Join-Path $CoreSource "include\twitchsdk.hpp"))) {
    if (-not (Test-Path -LiteralPath $required)) {
        Write-Error "Not found: $required"
        exit 1
    }
}

$destination = Join-Path $PSScriptRoot "Plugins\TwitchSDK"

function Invoke-Mirror([string]$from, [string]$to, [string[]]$excludeDirs) {
    # /MIR keeps the destination in sync (removes files deleted from the source).
    # Excluded folders are neither copied nor purged.
    $roboArgs = @($from, $to, "/MIR", "/NFL", "/NDL", "/NJH", "/NJS", "/NP")
    if ($excludeDirs.Count -gt 0) { $roboArgs += "/XD"; $roboArgs += $excludeDirs }
    & robocopy @roboArgs | Out-Null
    # robocopy exit codes 0-7 mean success (with various "files copied" details); 8+ is failure.
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed copying $from (exit $LASTEXITCODE)." }
}

Write-Host "Copying the plugin from $PluginSource ..."
# Build outputs are excluded; "core" is excluded here because it's mirrored separately below.
Invoke-Mirror $PluginSource $destination @("Binaries", "Intermediate", "core")

Write-Host "Bundling the core library from $CoreSource ..."
Invoke-Mirror $CoreSource (Join-Path $destination "core") @("out", ".vs", "build", "docs")

$sourceFiles = (Get-ChildItem -LiteralPath $destination -Recurse -File | Where-Object { $_.Extension -in ".h", ".hpp", ".cpp", ".cs", ".in" }).Count
Write-Host "Plugin installed at $destination ($sourceFiles source files)."
exit 0
