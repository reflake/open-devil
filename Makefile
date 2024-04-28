# Compiler
CXX = g++

# Compilation flags (O2 - to compile faster)
CXXFLAGS = -g -Wall -std=c++17 -O2

# Execution file target
TARGET = main

# List of source files
SOURCES = *.cpp vulkan/*.cpp

# Vulkan support
VULKAN = -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -lSDL2 ${VULKAN} -o $(TARGET)

clean:
	rm -f $(TARGET)