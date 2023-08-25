@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM for 32 bit
REM cl  -MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DRAAVANAN_INTERNAL=1 -FC -Z7 -Fmwin32_RaavananTheHero.map ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link -subsystem:windows,5.1  -opt:ref user32.lib Gdi32.lib
REM for 64 bit
cl  -MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DRAAVANAN_INTERNAL=1 -FC -Z7 -Fmwin32_RaavananTheHero.map ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link -opt:ref user32.lib Gdi32.lib Winmm.lib
popd    