<#
.SYNOPSIS
    Installs the Twitch SDK plugin into this sample project from the shipped .unitypackage.

.DESCRIPTION
    A .unitypackage is a gzip tarball where each asset lives in a folder named after its GUID:
        <guid>/pathname    - the project-relative destination path (e.g. Assets/Plugins/TwitchSDK.dll)
        <guid>/asset       - the file contents (absent for folders)
        <guid>/asset.meta  - the Unity .meta file, which carries the GUID and import settings

    This script unpacks the package and places each asset and its .meta at its pathname, which is
    what Unity's own importer does. It does not launch the Unity Editor, so it works without an
    activated Editor license. Unity picks up the files the next time the project is opened.

.PARAMETER Package
    Path to the .unitypackage. Defaults to the one produced by unity-build\build.bat.
#>
param(
    [string]$Package = (Join-Path $PSScriptRoot "..\unity-build\TwitchSDK.unitypackage")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Package)) {
    Write-Error "Package not found: $Package. Run unity-build\build.bat first."
    exit 1
}

$projectRoot = $PSScriptRoot
$staging = Join-Path ([System.IO.Path]::GetTempPath()) ("twitchsdk-import-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $staging | Out-Null

try {
    # tar.exe ships with Windows 10+ and understands gzip tarballs.
    & tar -xzf $Package -C $staging
    if ($LASTEXITCODE -ne 0) { throw "tar failed to extract $Package (exit $LASTEXITCODE)." }

    $installed = 0
    foreach ($entry in Get-ChildItem -LiteralPath $staging -Directory) {
        $pathnameFile = Join-Path $entry.FullName "pathname"
        if (-not (Test-Path -LiteralPath $pathnameFile)) { continue }

        # The first line is the destination; later lines (if any) are not part of the path.
        $relative = (Get-Content -LiteralPath $pathnameFile -TotalCount 1).Trim()

        # Only allow destinations inside Assets/ so a malformed package can't write elsewhere.
        if ($relative -notmatch '^Assets(/|$)' -or $relative -match '(^|/)\.\.(/|$)') {
            Write-Warning "Skipping entry with unexpected path: $relative"
            continue
        }

        $destination = Join-Path $projectRoot ($relative -replace '/', '\')
        $assetFile = Join-Path $entry.FullName "asset"
        $metaFile = Join-Path $entry.FullName "asset.meta"

        if (Test-Path -LiteralPath $assetFile) {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
            Copy-Item -LiteralPath $assetFile -Destination $destination -Force
        }
        else {
            # Entries without an asset file are folders.
            New-Item -ItemType Directory -Force -Path $destination | Out-Null
        }

        if (Test-Path -LiteralPath $metaFile) {
            Copy-Item -LiteralPath $metaFile -Destination ($destination + ".meta") -Force
        }

        $installed++
        Write-Host "  $relative"
    }

    Write-Host "Installed $installed assets from $(Split-Path -Leaf $Package)."
}
finally {
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
}
