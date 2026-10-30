# Binary Chess Engine — build and test
#
#   make            build the engine, the perft runner and the rules test
#   make test       run the perft suite and the notation/rules suite
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
RULES     := $(BUILD_DIR)/rules$(EXE)

ENGINE_SOURCES := src/main.cpp src/board.cpp src/movegen.cpp src/eval.cpp src/search.cpp \
                  src/notation.cpp src/gamestate.cpp src/play.cpp
ENGINE_OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(ENGINE_SOURCES))
PERFT_OBJECTS  := $(BUILD_DIR)/perft.o $(BUILD_DIR)/board.o $(BUILD_DIR)/movegen.o
RULES_OBJECTS  := $(BUILD_DIR)/rules.o $(BUILD_DIR)/board.o $(BUILD_DIR)/movegen.o \
                  $(BUILD_DIR)/notation.o $(BUILD_DIR)/gamestate.o

CXXFLAGS += -MMD -MP
DEPS := $(ENGINE_OBJECTS:.o=.d) $(PERFT_OBJECTS:.o=.d) $(RULES_OBJECTS:.o=.d)

.PHONY: all engine test clean

all: engine $(PERFT) $(RULES)

engine: $(ENGINE)

$(ENGINE): $(ENGINE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(PERFT): $(PERFT_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(RULES): $(RULES_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/perft.o: tests/perft.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/rules.o: tests/rules.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

test: $(PERFT) $(RULES)
	$(PERFT)
	$(RULES)

ifeq ($(OS),Windows_NT)
clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
clean:
	@rm -rf $(BUILD_DIR)
endif

-include $(DEPS)
