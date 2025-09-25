# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -pthread -g3

# Target executable
TARGET = mutex_bench

# Source files
SOURCES = main.cpp utils.cpp
HEADERS = mutexes.hpp utils.hpp

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SOURCES) $(HEADERS)
	NIX_ENFORCE_NO_NATIVE=0 $(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Disassemble target
disassemble: $(TARGET)
	objdump -d $(TARGET) > $(TARGET).asm

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all run clean