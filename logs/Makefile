CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -I../process-manager/third-party -Ithird-party/spdlog/include
LDFLAGS := -pthread

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := ../bin
LIB_TARGET := libdacc-logs.a
BIN_TARGET := $(BIN_DIR)/log-server

SRCS_LIB := $(SRC_DIR)/LogManager.cpp $(SRC_DIR)/UnixSocketClient.cpp
SRCS_SERVER := $(SRC_DIR)/LogServer.cpp

OBJS_LIB := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
OBJS_SERVER := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_SERVER))

.PHONY: all clean

all: $(LIB_TARGET) $(BIN_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	ar rcs $@ $^

$(BIN_TARGET): $(OBJS_SERVER) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_SERVER) -L. -ldacc-logs -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

-include $(BUILD_DIR)/*.d

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(BIN_TARGET)
