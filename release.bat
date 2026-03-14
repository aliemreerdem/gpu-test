@echo off
echo =======================================
echo GPU Stress Tester - Release Packager
echo =======================================

echo 1. Ensuring environment is set and compiling latest code...
set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% (
    call %VCVARS%
    call build.bat
) else (
    echo [ERROR] "vcvars64.bat not found at %VCVARS%"
    echo Cannot compile Release.
    pause
    exit /b 1
)

echo.
echo 2. Packaging Game.exe and /shaders directory into ZIP archive...
if exist GPU_Stress_Tester_Release.zip (
    del GPU_Stress_Tester_Release.zip
)

powershell -Command "Compress-Archive -Path Game.exe, shaders -DestinationPath GPU_Stress_Tester_Release.zip -Force"

echo.
echo [SUCCESS] Release package created: GPU_Stress_Tester_Release.zip
echo You can now copy this ZIP file to any Windows computer and run Game.exe!
pause
