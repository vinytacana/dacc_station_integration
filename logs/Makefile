CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -I../ipc/include -I../process-manager/third-party -Ithird-party/spdlog/include
LDFLAGS := -pthread -L../ipc -ldacc-ipc

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := ../bin
LIB_TARGET := libdacc-logs.a
BIN_TARGET := $(BIN_DIR)/log-server
IPC_LIB := ../ipc/libdacc-ipc.a
TEST_BIN := tests/bin/test_log_protocol

SRCS_LIB := $(SRC_DIR)/LogManager.cpp $(SRC_DIR)/UnixSocketClient.cpp $(SRC_DIR)/LogProtocol.cpp
SRCS_SERVER := $(SRC_DIR)/LogServer.cpp

OBJS_LIB := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
OBJS_SERVER := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_SERVER))

.PHONY: all clean test

all: $(IPC_LIB) $(LIB_TARGET) $(BIN_TARGET)

$(IPC_LIB): $(wildcard ../ipc/src/*.cpp ../ipc/include/ipc/*.hpp)
	$(MAKE) -C ../ipc

$(LIB_TARGET): $(OBJS_LIB)
	ar rcs $@ $^

$(BIN_TARGET): $(OBJS_SERVER) $(LIB_TARGET) $(IPC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_SERVER) -L. -ldacc-logs -o $@ $(LDFLAGS)

$(TEST_BIN): tests/test_log_protocol.cpp $(LIB_TARGET) $(IPC_LIB)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(LIB_TARGET) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

-include $(BUILD_DIR)/*.d

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(BIN_TARGET) $(TEST_BIN)
