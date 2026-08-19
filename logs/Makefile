CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -I../ipc/include -I../process-manager/third-party -Ithird-party/spdlog/include
LDFLAGS := -pthread -L../ipc -ldacc-ipc

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := ../bin
LIB_TARGET := libdacc-logs.a
BIN_TARGET := $(BIN_DIR)/log-server
IPC_LIB := ../ipc/libdacc-ipc.a

SRCS_LIB := $(SRC_DIR)/LogManager.cpp $(SRC_DIR)/UnixSocketClient.cpp
SRCS_SERVER := $(SRC_DIR)/LogServer.cpp

OBJS_LIB := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
OBJS_SERVER := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS_SERVER))

.PHONY: all clean

all: $(IPC_LIB) $(LIB_TARGET) $(BIN_TARGET)

$(IPC_LIB): ../ipc/src/FramedSocket.cpp ../ipc/include/ipc/FramedSocket.hpp
	$(MAKE) -C ../ipc

$(LIB_TARGET): $(OBJS_LIB)
	ar rcs $@ $^

$(BIN_TARGET): $(OBJS_SERVER) $(LIB_TARGET) $(IPC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_SERVER) -L. -ldacc-logs -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

-include $(BUILD_DIR)/*.d

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(BIN_TARGET)
