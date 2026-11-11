CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -g -MMD -MP -Iinclude
LDLIBS   := -lX11

SRC := $(wildcard src/*.cpp src/gfx/*.cpp src/game/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
BIN := build/rpg

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

-include $(OBJ:.o=.d)

.PHONY: all run clean
