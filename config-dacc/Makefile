CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I.
AR := ar
ARFLAGS := rcs


LIB_TARGET := libdacc-config.a

SRCS_LIB := HardwareControl.cpp
OBJS_LIB := $(SRCS_LIB:.cpp=.o)

.PHONY: all clean

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIB_TARGET)