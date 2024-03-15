CXX = g++
CXXFLAGS = -c -Wall -g -std=c++17
LDFLAGS = -lpthread

TARGETS = mutex_lock unique_lock
SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

all: $(TARGETS)

$(TARGETS): %: %.o
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGETS)
