@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -DRAAVANAN_INTERNAL=1 -FC -Zi ..\VS_Code\RaavananTheHero\win32_Raavananthehero.cpp user32.lib Gdi32.lib
popd