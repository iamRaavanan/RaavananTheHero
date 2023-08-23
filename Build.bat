@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -MT -nologo -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DRAAVANAN_INTERNAL=1 -FC -Z7 -Fmwin32_RaavananTheHero.map ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib Gdi32.lib
popd