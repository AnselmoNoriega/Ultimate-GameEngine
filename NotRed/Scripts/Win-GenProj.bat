@echo off
pushd ..\
call vendor\premake\bind\premake5.exe vs2022
pushd NotEditor\SandboxProject
call Win-CreateScriptProjects.bat
popd
popd
PAUSE