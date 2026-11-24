@echo off
rem Copies the TwitchSDK Godot addon into both sample projects (gdscript and csharp).
rem Build the addon first with TwitchSDK-Godot\build.bat. See import-plugin.ps1 for details.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0import-plugin.ps1" %*
exit /b %ERRORLEVEL%
