<#
.SYNOPSIS
    Copies the TwitchSDK Godot addon into both sample projects.
.DESCRIPTION
    Mirrors TwitchSDK-Godot\addons\twitchsdk into gdscript\addons\twitchsdk and csharp\addons\twitchsdk.
    The GDScript sample doesn't get the C# wrappers (it isn't a .NET project).
    Build the addon first with TwitchSDK-Godot\build.bat. Re-run this after rebuilding it.
#>
param(
    [string]$AddonSource = (Join-Path $PSScriptRoot "..\TwitchSDK-Godot\addons\twitchsdk")
)
$ErrorActionPreference = "Stop"

$gdextension = Join-Path $AddonSource "twitchsdk.gdextension"
$library = Join-Path $AddonSource "bin\libtwitchsdk.windows.template_debug.x86_64.dll"
foreach ($required in @($gdextension, $library)) {
    if (-not (Test-Path -LiteralPath $required)) {
        Write-Error "Not found: $required. Build the addon first with TwitchSDK-Godot\build.bat."
    }
}

function Invoke-Mirror([string]$From, [string]$To, [string[]]$ExcludeDirs) {
    $robocopyArgs = @($From, $To, "/MIR", "/NFL", "/NDL", "/NJH", "/NJS", "/NP")
    if ($ExcludeDirs) { $robocopyArgs += "/XD"; $robocopyArgs += $ExcludeDirs }
    & robocopy @robocopyArgs | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE) copying to $To" }
}

$gdscriptDest = Join-Path $PSScriptRoot "gdscript\addons\twitchsdk"
$csharpDest = Join-Path $PSScriptRoot "csharp\addons\twitchsdk"

Write-Host "Copying the addon from $AddonSource ..."
Invoke-Mirror $AddonSource $gdscriptDest @("csharp")
Invoke-Mirror $AddonSource $csharpDest @()

Write-Host "Done. The addon is in:"
Write-Host "  $gdscriptDest"
Write-Host "  $csharpDest"
exit 0
