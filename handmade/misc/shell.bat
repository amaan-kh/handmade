@echo off
call C:
call cd C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\
call vcvarsall.bat x64
call W:
set path=w:\handmade\misc;%path%
