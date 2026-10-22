@echo off
rem Builds the engine to WebAssembly: web\build\engine.js + engine.wasm

rem Load Emscripten into this window, in case the PATH isn't set up.
call C:\emsdk\emsdk_env.bat >nul

set EMSDK_QUIET=1
set TEMP=%~dp0build\tmp
set TMP=%~dp0build\tmp
if not exist "%TEMP%" mkdir "%TEMP%"

rem Run from the project root, wherever this script was started from.
cd /d "%~dp0.."
if not exist web\build mkdir web\build

em++ -O3 -std=c++20 -fexceptions ^
  -I engine ^
  web\engine_web.cpp ^
  engine\position.cpp engine\movegen.cpp engine\evaluate.cpp ^
  engine\search.cpp engine\perft.cpp ^
  -lembind ^
  -sMODULARIZE=1 -sEXPORT_NAME=createEngine ^
  -sENVIRONMENT=web,worker,node ^
  -sSTACK_SIZE=4MB -sALLOW_MEMORY_GROWTH=1 ^
  -o web\build\engine.js

if errorlevel 1 (echo BUILD FAILED) else (echo Built web\build\engine.js and engine.wasm)