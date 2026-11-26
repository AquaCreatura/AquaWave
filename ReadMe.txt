"C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\windeployqt.exe" для деплоя
Сборка x64 Debug: build-x64-debug.bat (VS2022, v143, SDK 10.0) → exe: x64\Debug\AquaWave.exe

Console mode (full reference: AquaWave.exe --help):
  Commands:  -dmd  demodulate a signal file;  -mod  modulate a data file.
  Common:    --type <Type>  --in <File>  (--out <File>, -mod only).
  Demod:     --file_fc <Hz> / --file_fs <Hz>  (optional, taken from the file name)
             --dmd_fc <Hz> / --dmd_sr <Hz>    (required)
             --pll <v> / --tll <v> / --eq <v> (loop speeds, optional)
             --eq_alg <AUTO|MMA|CMA|LMS> / --eq_taps <n> (equaliser, optional)
  Mod:       --sym_rate <Hz>  --noise <dB>  --out_size <bytes>

  Examples:
    AquaWave.exe -dmd --type QPSK --in "iDirect 11081.434200MHz 7000.000KHz.pcm" --dmd_fc 11081405991 --dmd_sr 297454
    AquaWave.exe -mod --type QAM32 --in data.bin --out qam32.pcm --noise -30

  Test sets: ConsoleProcessing\DmdCcmTests.xml, ConsoleProcessing\ModulateCcm.xml
