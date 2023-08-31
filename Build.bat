@echo off

set CommonCompileFlags=-MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DRAAVANAN_INTERNAL=1 -DRAAVANAN_WIN32=1-FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM for 32 bit
REM cl  %CommonCompileFlags% ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp /link -subsystem:windows,5.1  -opt:ref user32.lib Gdi32.lib
REM for 64 bit
cl %CommonCompileFlags%  ..\VS_Code\RaavananTheHero\Raavanan.cpp -FmRaavanan.map /LD /link /DLL /EXPORT:GetGameSoundSamples /EXPORT:GameUpdateAndRender
cl %CommonCompileFlags%  ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp -Fmwin32_RaavananTheHero.map /link %CommonLinkerFlags%
popd    