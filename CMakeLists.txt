cmake_minimum_required(VERSION 3.20)
project(chess CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The engine: no main(), no GUI code. Everything else links against this.
add_library(engine STATIC
        engine/position.cpp
        engine/movegen.cpp
        engine/evaluate.cpp
        engine/search.cpp
        engine/perft.cpp
)
target_include_directories(engine PUBLIC engine)

# Tests: one executable each, since each has its own main().
add_executable(position_tests tests/position_tests.cpp)
target_link_libraries(position_tests PRIVATE engine)

add_executable(perft_tests tests/perft_tests.cpp)
target_link_libraries(perft_tests PRIVATE engine)

# UCI engine program. Uncomment once uci/main.cpp has a main().
# add_executable(chess_uci uci/main.cpp)
# target_link_libraries(chess_uci PRIVATE engine)

# wxWidgets GUI. Uncomment once the GUI is written; it needs wxWidgets linked too.
# add_executable(chess_gui WIN32 gui/App.cpp gui/MainFrame.cpp gui/BoardView.cpp
#                gui/Game.cpp gui/EngineWorker.cpp)
# target_link_libraries(chess_gui PRIVATE engine)