CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. -MMD -MP
AR := ar
ARFLAGS := rcs


LIB_TARGET := libdacc-config.a

SRCS_LIB := HardwareControl.cpp ConfigCommand.cpp BluetoothInternal.cpp BluetoothControl.cpp
OBJS_LIB := $(SRCS_LIB:.cpp=.o)
DEPS_LIB := $(OBJS_LIB:.o=.d)

.PHONY: all clean

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.d $(LIB_TARGET)

-include $(DEPS_LIB)
