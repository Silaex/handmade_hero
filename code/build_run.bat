@echo off
mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\handmade\code\win32_handmade.cpp gdi32.lib user32.lib
popd
echo Game is running...
..\..\build\win32_handmade.exe
