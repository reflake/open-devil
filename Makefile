ifeq "$(CONFIG)" ""
	CONFIG=Release
endif

# Build directory
BUILD_DIR = $(CONFIG)

# Compiler
CXX = g++

# Compilation flags (O2 - to compile faster)
CXXFLAGS = -std=c++17 -O2

ifeq "$(CONFIG)" "Debug"
	CXXFLAGS += -g -Wall
endif

# Linker flags
LDXXFLAGS = -lm

# Execution file target
TARGET = main
TARGET_BUILD_PATH = $(BUILD_DIR)/$(TARGET)

# Sources
SOURCE_DIR = src

# Objects
OBJECTS = \
	$(BUILD_OBJ_DIR)/main.o \
	$(BUILD_OBJ_DIR)/file.o \
	$(BUILD_OBJ_DIR)/application.o \
	$(BUILD_OBJ_DIR)/vulkan/shader.o \
	$(BUILD_OBJ_DIR)/vulkan/engine.o \
	$(BUILD_OBJ_DIR)/vulkan/types/qfamily_indices.o \
	$(BUILD_OBJ_DIR)/vulkan/types/swap_chain_support.o

OBJECT_DIRS = \
	$(BUILD_OBJ_DIR)/vulkan \
	$(BUILD_OBJ_DIR)/vulkan/types \

BUILD_OBJ_DIR = $(BUILD_DIR)/obj

# Vulkan support
VULKAN = -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# SDL support
SDL = -lSDL2

COMP = $(CXX) $(CXXFLAGS) -o $@ -c $<

# Shaders
SHADERS = \
	$(BUILD_SHADER_DIR)/main.frag.spv \
	$(BUILD_SHADER_DIR)/main.vert.spv \

BUILD_SHADER_DIR = $(BUILD_DIR)/shaders

SHADER_SRC_DIR = shaders

all: dirs $(TARGET_BUILD_PATH)

$(TARGET_BUILD_PATH): $(OBJECTS) $(SHADERS)
	$(CXX) $(OBJECTS) $(LDXXFLAGS) $(SDL) $(VULKAN) -o $(TARGET_BUILD_PATH)

dirs:
	-mkdir -p $(BUILD_DIR)
	-mkdir -p $(BUILD_OBJ_DIR)
	-mkdir -p $(OBJECT_DIRS)
	-mkdir -p $(BUILD_SHADER_DIR)
	$(MV_SHADERS)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(COMP)

$(BUILD_SHADER_DIR)/%.spv : $(SHADER_SRC_DIR)/%
	glslc $< -o $@