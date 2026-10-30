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
    src/notation.cpp
    src/gamestate.cpp
    src/play.cpp
)

# Engine executable
find_package(Threads REQUIRED)
add_executable(chess-engine ${SOURCES})
target_include_directories(chess-engine PRIVATE src)
target_link_libraries(chess-engine PRIVATE Threads::Threads)

# Perft test runner
add_executable(perft
    tests/perft.cpp
    src/board.cpp
    src/movegen.cpp
)
target_include_directories(perft PRIVATE src)

# Notation and game-rules test runner
add_executable(rules
    tests/rules.cpp
    src/board.cpp
    src/movegen.cpp
    src/notation.cpp
    src/gamestate.cpp
)
target_include_directories(rules PRIVATE src)

# Search limits, horizon and quiescence tests
add_executable(searchtest
    tests/search.cpp
    src/board.cpp
    src/movegen.cpp
    src/eval.cpp
    src/search.cpp
)
target_include_directories(searchtest PRIVATE src)
target_link_libraries(searchtest PRIVATE Threads::Threads)

# Compiler flags. Deliberately no -march=native: the binary stays portable.
target_compile_options(chess-engine PRIVATE -O2 -Wall -Wextra -Wpedantic)
target_compile_options(perft PRIVATE -O2 -Wall -Wextra -Wpedantic)
target_compile_options(rules PRIVATE -O2 -Wall -Wextra -Wpedantic)
target_compile_options(searchtest PRIVATE -O2 -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME perft COMMAND perft)
add_test(NAME rules COMMAND rules)
add_test(NAME searchtest COMMAND searchtest)
