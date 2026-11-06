@echo off
REM ============================================================
REM  2far Bitcrusher - compilar en Windows
REM  Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo
REM  para el escritorio con C++"), CMake y Git en el PATH.
REM ============================================================
setlocal

cmake -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :error

cmake --build build --config Release --target TwoFarBitcrusher_VST3
if errorlevel 1 goto :error

echo.
echo Listo. El plugin esta en:
echo   build\TwoFarBitcrusher_artefacts\Release\VST3\2far Bitcrusher.vst3
echo.
echo Para usarlo en FL Studio copia la carpeta "2far Bitcrusher.vst3" a:
echo   C:\Program Files\Common Files\VST3
echo (necesita permisos de administrador) y en FL Studio ve a
echo Options ^> Manage plugins ^> Find installed plugins.
goto :eof

:error
echo.
echo Algo fallo durante la compilacion. Revisa los mensajes de arriba.
exit /b 1
