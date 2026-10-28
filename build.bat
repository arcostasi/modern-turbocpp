@echo off
setlocal
echo ========================================================
echo  Building Modern Turbo C++ (C++23) with LLVM Clang
echo ========================================================

set LLVM_BIN=C:\LLVM\bin
set CXX=%LLVM_BIN%\clang++.exe
set CC=%LLVM_BIN%\clang.exe

if not exist "%CXX%" (
    echo Error: LLVM Clang compiler not found at %CXX%
    exit /b 1
)

if not exist build (
    mkdir build
)

cmake -B build -G Ninja ^
    -DCMAKE_CXX_COMPILER="%CXX%" ^
    -DCMAKE_C_COMPILER="%CC%" ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

cmake --build build --config Release

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo ========================================================
echo  Modern Turbo C++ built successfully!
echo  Executable: bin\turboc.exe
echo ========================================================
endlocal
