CXX = g++
CXXFLAGS = -c -Wall -g
LDFLAGS = -lpthread

TARGETS = Thread_Management
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
