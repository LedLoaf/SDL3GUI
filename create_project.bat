@echo off
setlocal

set VCPKG=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
set SRC=%~dp0
set SRC=%SRC:~0,-1%
set BUILD=%SRC%\build
set DEPS=%SRC%\dependencies

:: --- Pass 1: Configure with vcpkg to populate vcpkg_installed ---
if exist "%BUILD%\CMakeCache.txt" del "%BUILD%\CMakeCache.txt"

cmake -B "%BUILD%" -S "%SRC%" -DCMAKE_TOOLCHAIN_FILE=%VCPKG%
if %errorlevel% neq 0 (
    echo [ERROR] Vcpkg configure failed.
    pause
    exit /b 1
)

if not exist "%BUILD%\vcpkg_installed" (
    echo [ERROR] vcpkg_installed not found. Is vcpkg.json present?
    pause
    exit /b 1
)

:: --- Move vcpkg_installed to Dependencies ---
if exist "%DEPS%" rmdir /S /Q "%DEPS%"
move "%BUILD%\vcpkg_installed" "%DEPS%"
if %errorlevel% neq 0 (
    echo [ERROR] Move to Dependencies/ failed.
    pause
    exit /b 1
)

:: --- Pass 2: Reconfigure WITHOUT vcpkg so .vcxproj files point to Dependencies/ ---
del "%BUILD%\CMakeCache.txt"
cmake -B "%BUILD%" -S "%SRC%"
if %errorlevel% neq 0 (
    echo [ERROR] Final configure failed.
    pause
    exit /b 1
)

:: --- Replace absolute Dependencies path with $(SolutionDir)..\Dependencies ---
for /R "%BUILD%" %%f in (*.vcxproj) do (
    powershell -NoProfile -Command "(Get-Content '%%f' -Raw) -replace [regex]::Escape('%DEPS%'), '$(SolutionDir)..\Dependencies' | Set-Content '%%f' -NoNewline"
)

echo [OK] Done. Open the solution in VS.
pause   