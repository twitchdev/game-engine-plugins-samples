@echo off
rem Installs the Twitch SDK plugin into this sample project from the package that
rem unity-build\build.bat produces. Re-run this after rebuilding the plugin so the
rem sample always tests the latest shipped package. See import-plugin.ps1 for details.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0import-plugin.ps1" %*
exit /b %ERRORLEVEL%
