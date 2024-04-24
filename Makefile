# Compiler
CXX = g++

# Compilation flags
CXXFLAGS = -g -Wall

# Execution file target
TARGET = main

# List of source files
SOURCES = *.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -lSDL2 -o $(TARGET)

clean:
	rm -f $(TARGET)