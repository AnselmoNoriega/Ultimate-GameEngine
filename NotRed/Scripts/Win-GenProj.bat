@echo off
pushd ..\
call vendor\bind\premake\premake5.exe vs2022
popd
PAUSE