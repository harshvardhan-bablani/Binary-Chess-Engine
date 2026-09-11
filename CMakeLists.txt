cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 17)

# Source files
set(SOURCES
    src/main.cpp
    src/board.cpp
    src/movegen.cpp
    src/eval.cpp
    src/search.cpp
)

# Create executable
add_executable(chess-engine ${SOURCES})

# Include directory
target_include_directories(chess-engine PRIVATE src)

# Compiler flags
if(MSVC)
    target_compile_options(chess-engine PRIVATE /O2)
else()
    target_compile_options(chess-engine PRIVATE -O2 -march=native)
endif()
