@echo off
set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% (
    call %VCVARS%
    call build.bat
    echo Done.
) else (
    echo "vcvars64.bat not found at %VCVARS%"
)
