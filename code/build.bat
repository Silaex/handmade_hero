@echo off
mkdir ..\..\build
pushd ..\..\build
cl -FC -Zi ..\handmade\code\win32_handmade.cpp gdi32.lib user32.lib
popd