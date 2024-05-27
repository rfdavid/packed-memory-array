CXXFLAGS := -std=c++20 -Wall -O0 -g

all: pma

pma: pma.cpp
	$(CXX) pma.cpp -o pma $(CXXFLAGS)

clean:
	rm -f pma
