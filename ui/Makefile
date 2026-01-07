CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -I../process-manager/third-party -I../logs/include -I../config-dacc
LDFLAGS := -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer -L../logs -ldacc-logs -L../config-dacc -ldacc-config

# Diretórios
BIN_DIR := ../bin
BUILD_DIR := build
SRC_DIR := src

# Alvo
TARGET := $(BIN_DIR)/dacc-ui

# Fontes e Objetos
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Regra padrão
all: $(TARGET)

# Linkagem
$(TARGET): $(OBJS)
	@echo "==> Ligando UI: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compilação
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "==> Compilando: $<"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

# Include de dependências (.d)
-include $(BUILD_DIR)/*.d

.PHONY: clean all

clean:
	@echo "==> Limpando UI..."
	rm -rf $(BUILD_DIR) $(TARGET)
