@echo off

:: Step 1: Run the Python script to set the environment variable
:: cd instalations
:: python install_fbx_sdk.py

:: Step 2: Read the output from the Python script and set the environment variable
:: set /p FBX_SDK_PATH=<fbx_sdk_path.txt

:: Step 3: Navigate back to the main directory and run Premake using the same terminal session
:: cd .. 
pushd ..\
call vendor\premake\bind\premake5.exe vs2022

:: Step 5: Run your project-specific batch file or commands
pushd NotEditor\SandboxProject
call Win-CreateScriptProjects.bat
popd

popd

PAUSE