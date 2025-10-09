@echo off
set "qt_dir=C:\Qt\Qt5.14.2\5.14.2\msvc2017_64"
set "exe_path_debug=D:\PetWave\AquaWave\x64\Debug\AquaWave.exe"  <-- Убедитесь, что это правильный путь!
set "windeployqt_path=%qt_dir%\bin\windeployqt.exe"

echo Setting up Visual Studio environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Deploying DLLs for Debug configuration...
"%windeployqt_path%" --debug "%exe_path_debug%"

pause