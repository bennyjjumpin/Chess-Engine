cmake_minimum_required(VERSION 3.20)
project(chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chess engine/types.h
        engine/types.h
        engine/move.h
        engine/position.cpp
        engine/position.h
        engine/movegen.cpp
        engine/movegen.h
        engine/evaluate.cpp
        engine/evaluate.h
        engine/search.cpp
        engine/search.h
        uci/main.cpp
        uci/main.cpp
        gui/App.cpp
        gui/App.h
        gui/MainFrame.cpp
        gui/MainFrame.h
        gui/BoardView.cpp
        gui/BoardView.h
        gui/Game.cpp
        gui/Game.h
        gui/EngineWorker.cpp
        gui/EngineWorker.h
        tests/perft.cpp
        tests/position_tests.cpp)