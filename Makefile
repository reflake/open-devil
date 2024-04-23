# Compiler
CXX = g++

# Compilation flags
CXXFLAGS = -g -Wall

# Execution file target
TARGET = main

# List of source files
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)