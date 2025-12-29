@echo off
@REM cmake --build out/build/gcc-debug
@REM .\out\build\gcc-debug\RobloxEng.exe

set VCPKG_PATH=E:\vcpkg-2025.12.12

:: 1. Configura
cmake --preset gcc-debug ^
 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake" ^
 -DVCPKG_TARGET_TRIPLET=x64-mingw-static

:: 2. Compila
cmake --build out/build/gcc-debug

:: 3. Executa se o build for 100%
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCESSO] Rodando...
    .\out\build\gcc-debug\RobloxEng.exe
) else (
    echo.
    echo [ERRO] A compilação falhou.
    pause
)