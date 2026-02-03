
# Local: Faculdade/dacc_station/dacc-station-os/ui/Makefile

CXX = g++

# Includes (Onde achar os .hpp)
CXXFLAGS = -std=c++17 -Wall -Wextra -g \
           -Iinclude \
           -I../config-dacc \
           -I../logs/include \
           -I../process-manager/third-party \
           -I../logs/third-party/spdlog/include

# Bibliotecas (Onde achar os .a e quais linkar)
LDFLAGS = -L../config-dacc -ldacc-config \
          -L../logs -ldacc-logs \
          -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = ../bin
BIN = $(BIN_DIR)/dacc-ui

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

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