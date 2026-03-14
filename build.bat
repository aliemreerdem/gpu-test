@echo off
echo =======================================
echo GPU Stress Tester - Build Script
echo =======================================

echo Compiling Compute Shaders offline (FXC.exe)...
fxc.exe /nologo /E CSMath /T cs_5_0 /Fo kernel_1.cso stress.hlsl
fxc.exe /nologo /E CSMemory /T cs_5_0 /Fo kernel_2.cso stress.hlsl
fxc.exe /nologo /E CSGame /T cs_5_0 /Fo kernel_3.cso stress.hlsl
fxc.exe /nologo /E CSCrypto /T cs_5_0 /Fo kernel_4.cso stress.hlsl
fxc.exe /nologo /E CSRayTrace /T cs_5_0 /Fo kernel_5.cso stress.hlsl
fxc.exe /nologo /E CSMemoryMassive /T cs_5_0 /Fo kernel_6.cso stress.hlsl
fxc.exe /nologo /E CSParticles /T cs_5_0 /Fo kernel_7.cso stress.hlsl
fxc.exe /nologo /E CSCache /T cs_5_0 /Fo kernel_8.cso stress.hlsl
fxc.exe /nologo /E CSDeferred /T cs_5_0 /Fo kernel_9.cso stress.hlsl
fxc.exe /nologo /E CSRedDead2 /T cs_5_0 /Fo kernel_10.cso stress.hlsl
fxc.exe /nologo /E CSDiablo4 /T cs_5_0 /Fo kernel_11.cso stress.hlsl
fxc.exe /nologo /E VSMain /T vs_5_0 /Fo vs_dummy.cso dummy.hlsl
fxc.exe /nologo /E PSMain /T ps_5_0 /Fo ps_dummy.cso dummy.hlsl
echo.

echo Compiling main.cpp...
cl.exe /EHsc /W4 /Ox /nologo main.cpp /link /OUT:Game.exe d3d11.lib dxgi.lib user32.lib

if %errorlevel% neq 0 goto :error
echo.
echo [SUCCESS] Build completed successfully.
echo Run "Game.exe" to start the stress test.
echo To stop the test, press Ctrl+C in the console window.
goto :EOF

:error
echo.
echo [ERROR] Build failed. 
echo Ensure you are running this from a 'x64 Native Tools Command Prompt for VS 2022' (or similar).
goto :EOF

