# Makefile para dacc-ui
# Padronizado para o ecossistema DACC Station

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude -I../logs/include -I../process-manager/third-party -I../logs/third-party/spdlog/include
LDFLAGS = -L../logs -ldacc-logs -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer

# Diretórios
SRC_DIR = src
OBJ_DIR = build
BIN_DIR = ../bin
BIN = $(BIN_DIR)/dacc-ui

# Arquivos
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Regras
all: directories $(BIN)

$(BIN): $(OBJS)
	@echo "Linking $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)

rebuild: clean all

.PHONY: all clean rebuild directories