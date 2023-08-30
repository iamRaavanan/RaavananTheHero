@echo off

set CommonCompileFlags=-MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DRAAVANAN_INTERNAL=1 -FC -Z7 -Fmwin32_RaavananTheHero.map
set CommonLinkerFlags=-opt:ref user32.lib Gdi32.lib Winmm.lib
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM for 32 bit
REM cl  %CommonCompileFlags% ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link -subsystem:windows,5.1  -opt:ref user32.lib Gdi32.lib
REM for 64 bit
cl %CommonCompileFlags%  ..\VS_Code\RaavananTheHero\Raavanan.cpp /link /DLL
cl %CommonCompileFlags%  ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link %CommonLinkerFlags%
popd    