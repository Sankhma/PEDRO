@echo off

IF NOT EXIST .\build mkdir .\build
cd .\build

CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

@set SOURCES=..\main\main.cpp ..\main\imgui\imgui*.cpp
@set LIBS=User32.lib Ws2_32.lib d3d11.lib d3dcompiler.lib

cl /nologo /Zi /EHsc %SOURCES% /link %LIBS%