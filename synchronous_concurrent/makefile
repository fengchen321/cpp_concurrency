CXX = g++
CXXFLAGS = -c -Wall -g -std=c++11
LDFLAGS = -lpthread

TARGETS = condition_var
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
