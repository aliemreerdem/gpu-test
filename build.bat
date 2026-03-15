@echo off
echo =======================================
echo GPU Stress Tester - Build Script
echo =======================================

echo Compiling Compute Shaders offline (FXC.exe)...
if not exist shaders mkdir shaders
fxc.exe /nologo /E CSMath /T cs_5_0 /Fo shaders\kernel_1.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSMemory /T cs_5_0 /Fo shaders\kernel_2.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSGame /T cs_5_0 /Fo shaders\kernel_3.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSCrypto /T cs_5_0 /Fo shaders\kernel_4.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSRayTrace /T cs_5_0 /Fo shaders\kernel_5.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSMemoryMassive /T cs_5_0 /Fo shaders\kernel_6.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSParticles /T cs_5_0 /Fo shaders\kernel_7.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSCache /T cs_5_0 /Fo shaders\kernel_8.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSDeferred /T cs_5_0 /Fo shaders\kernel_9.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSRedDead2 /T cs_5_0 /Fo shaders\kernel_10.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E CSDiablo4 /T cs_5_0 /Fo shaders\kernel_11.cso src\hlsl\stress.hlsl
fxc.exe /nologo /E VSMain /T vs_5_0 /Fo shaders\vs_dummy.cso src\hlsl\dummy.hlsl
fxc.exe /nologo /E PSMain /T ps_5_0 /Fo shaders\ps_dummy.cso src\hlsl\dummy.hlsl
echo Compiling C++ OOP Architecture...
cl.exe /EHsc /W4 /Ox /MT /nologo main.cpp src\core\*.cpp src\graphics\*.cpp /link /OUT:Game.exe d3d11.lib dxgi.lib user32.lib

if %errorlevel% neq 0 goto :error
echo.
echo Cleaning up intermediate Object (.obj) files...
del *.obj
if exist src\core\*.obj del src\core\*.obj
if exist src\graphics\*.obj del src\graphics\*.obj
echo [SUCCESS] Build completed successfully.
echo Run "Game.exe" to start the stress test.
echo To stop the test, press Ctrl+C in the console window.
goto :EOF

:error
echo.
echo [ERROR] Build failed. 
echo Ensure you are running this from a 'x64 Native Tools Command Prompt for VS 2022' (or similar).
goto :EOF

