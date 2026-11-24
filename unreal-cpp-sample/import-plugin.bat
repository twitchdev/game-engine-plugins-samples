@echo off
rem Copies the Twitch SDK Unreal plugin (with the core library bundled) into this sample.
rem Re-run after changing the plugin or core. See import-plugin.ps1 for details.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0import-plugin.ps1" %*
exit /b %ERRORLEVEL%
