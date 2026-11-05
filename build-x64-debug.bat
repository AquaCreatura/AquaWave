@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe" "%~dp0AquaWave.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
set "rc=%errorlevel%"
echo.
echo exit code: %rc%
pause
exit /b %rc%
