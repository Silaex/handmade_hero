@echo off
mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\handmade\code\win32_handmade.cpp gdi32.lib user32.lib
popd