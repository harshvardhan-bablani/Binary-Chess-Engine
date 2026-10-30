# Binary Chess Engine — build and test
#
#   make            build the engine and the perft runner into build/
#   make test       run the perft suite
#   make clean      remove build/
#
# On Windows use `mingw32-make` (ships with MinGW/MSYS2), or just run
# build.bat and run_tests.bat.

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Isrc

BUILD_DIR := build
ENGINE    := $(BUILD_DIR)/chess-engine$(EXE)
PERFT     := $(BUILD_DIR)/perft$(EXE)

ENGINE_SOURCES := src/main.cpp src/board.cpp src/movegen.cpp src/eval.cpp src/search.cpp
ENGINE_OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(ENGINE_SOURCES))
PERFT_OBJECTS  := $(BUILD_DIR)/perft.o $(BUILD_DIR)/board.o $(BUILD_DIR)/movegen.o

CXXFLAGS += -MMD -MP
DEPS := $(ENGINE_OBJECTS:.o=.d) $(PERFT_OBJECTS:.o=.d)

.PHONY: all engine test clean

all: engine $(PERFT)

engine: $(ENGINE)

$(ENGINE): $(ENGINE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(PERFT): $(PERFT_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/perft.o: tests/perft.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

test: $(PERFT)
	$(PERFT)

ifeq ($(OS),Windows_NT)
clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
clean:
	@rm -rf $(BUILD_DIR)
endif

-include $(DEPS)
