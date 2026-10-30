cmake_minimum_required(VERSION 3.10)
project(ChessEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The engine uses __builtin_ctzll, which MSVC does not provide.
if(MSVC)
    message(FATAL_ERROR
        "This project requires GCC or Clang (it uses __builtin_ctzll). "
        "Build with MinGW/MSYS2 on Windows.")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/board.cpp
    src/movegen.cpp
    src/eval.cpp
    src/search.cpp
)

# Engine executable
add_executable(chess-engine ${SOURCES})
target_include_directories(chess-engine PRIVATE src)

# Perft test runner
add_executable(perft
    tests/perft.cpp
    src/board.cpp
    src/movegen.cpp
)
target_include_directories(perft PRIVATE src)

# Compiler flags. Deliberately no -march=native: the binary stays portable.
target_compile_options(chess-engine PRIVATE -O2 -Wall -Wextra -Wpedantic)
target_compile_options(perft PRIVATE -O2 -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME perft COMMAND perft)
