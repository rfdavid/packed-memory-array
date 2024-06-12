CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20

all: CXXFLAGS += -O2
all: pma

debug: CXXFLAGS += -O0 -g -DDEBUG  -fsanitize=address
debug: pma

pma: pma.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f pma
