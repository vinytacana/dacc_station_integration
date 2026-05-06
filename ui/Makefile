
# Local: Faculdade/dacc_station/dacc-station-os/ui/Makefile

CXX = g++

# Includes (Onde achar os .hpp)
CXXFLAGS = -std=c++17 -Wall -Wextra -g \
           -Iinclude \
           -I../config-dacc \
           -I../config-dacc/include \
           -I../logs/include \
           -I../process-manager/third-party \
           -isystem ../logs/third-party/spdlog/include \
           -MMD -MP

# Bibliotecas (Onde achar os .a e quais linkar)
LDFLAGS = ../config-dacc/libdacc-config.a \
          ../logs/libdacc-logs.a \
          -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = ../bin
BIN = $(BIN_DIR)/dacc-ui

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: directories $(BIN)

$(BIN): $(OBJS)
	@echo "Linkando dacc-ui..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compilando UI: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean directories

-include $(DEPS)
