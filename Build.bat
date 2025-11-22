@echo off

set CommonCompileFlags=-MTd -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4505 -wd4189 -DR_INTERNAL=0 -DR_WIN32=1-FC -Z7
set CommonLinkerFlags= -opt:ref user32.lib Gdi32.lib Winmm.lib
IF NOT EXIST ..\build mkdir ..\build
pushd ..\..\build
REM for 32 bit
REM cl  %CommonCompileFlags% ..\RaavananTheHero\win32_Raavananthehero.cpp /link -subsystem:windows,5.1  -opt:ref user32.lib Gdi32.lib
REM for 64 bit
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
set DATE=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
echo WAITING FOR PDB > lock.tmp
cl %CommonCompileFlags%  ..\RaavananTheHero\R.cpp -FmR.map /LD /link -incremental:no -PDB:R_%random%.pdb -EXPORT:GetGameSoundSamples -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompileFlags%  ..\RaavananTheHero\win32_Rthehero.cpp -Fmwin32_RTheHero.map /link %CommonLinkerFlags%
popd    