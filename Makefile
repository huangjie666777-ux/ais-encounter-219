CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pthread
INCLUDES := -I.deps/boost/usr/include -I.deps/json/include -Isrc
SRCS := $(wildcard src/*.cpp)
TARGET := ais_server

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS) -pthread

clean:
	rm -f $(TARGET)

.PHONY: clean
