CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20
LDFLAGS =

all: pma pma_key_value

debug: CXXFLAGS += -O0 -g -DDEBUG -fsanitize=address
debug: pma_debug pma_key_value_debug

pma: pma.cpp
	$(CXX) $(CXXFLAGS) -O2 -o $@ $< $(LDFLAGS)

pma_key_value: pma_key_value.cpp
	$(CXX) $(CXXFLAGS) -O2 -o $@ $< $(LDFLAGS)

pma_debug: pma.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

pma_key_value_debug: pma_key_value.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f pma pma_key_value pma_debug pma_key_value_debug
