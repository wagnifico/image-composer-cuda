################################################################################

################################################################################

# Define the compiler and flags
NVCC = nvcc

CXXFLAGS = -std=c++11
CXXFLAGS += -I./external/Common -I./external/Common/UtilNPP
CXXFLAGS += -I./external/FreeImage
CXXFLAGS += -I./src

LDFLAGS = -lcudart -lnppc -lnppial -lnppig -lnppisu
LDFLAGS += -lfreeimage

# Define directories
SRC_DIR = src
BIN_DIR = bin
DATA_DIR = data

# Define source files and target executable
SRCS = $(SRC_DIR)/main.cpp
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%.o)
TARGET = $(BIN_DIR)/main

# Define the default rule
all: $(TARGET)

# Get external code if not available
getCode:
	@if [ -d "external/Common" ]; then \
		echo "Found Common from cuda-samples"; \
	else \
		cd external; \
		git clone https://github.com/NVIDIA/cuda-samples; \
		mv cuda-samples/Common .; \
		rm -rf cuda-samples; \
		cd ..; \
	fi
	@if [ -d "external/FreeImage" ]; then \
		echo "Found FreeImage"; \
	else \
		cd external; \
		wget http://downloads.sourceforge.net/freeimage/FreeImage3180.zip; \
		unzip FreeImage3180.zip; \
		rm FreeImage3180.zip; \
		mv FreeImage/Source FreeImage; \
		rm -rf FreeImage; \
		cd ..; \
	fi

# Get data if not available, generate 1000px pngs
getData:
	@if [ -d "data/flags" ]; then \
		echo "Found flags"; \
	else \
		cd data; \
		git clone https://github.com/hampusborgos/country-flags.git; \
		npm install -g svgexport imagemin-cli ; \
		npm run build-pngs -- 1000: ; \
		cp -r png1000px flags; \
		cd ..; \
	fi

get: getCode getData


# Rules for building the target executable
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(NVCC) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	mkdir -p $(BIN_DIR)
	$(NVCC) $(OBJS) -o $@ $(LDFLAGS)

build: getCode $(TARGET)

# Rule for running the application - 2 examples
run: build getData
	./$(TARGET) --input "./data/colors" --width 105 --height 20
	#./$(TARGET) --width 1000 --alpha 0.1

# Clean up
clean:
	rm -rf $(BIN_DIR)/*

# Help command
help:
	@echo "Available make commands:"
	@echo "  make        - Build the project."
	@echo "  make get    - Get external dependencies (cuda-samples, FreeImage) and images."
	@echo "  make run    - Run the project."
	@echo "  make clean  - Clean up the build files."
	@echo "  make build  - Build the app."
	@echo "  make help   - Display this help message."
