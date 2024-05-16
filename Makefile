CXXFLAGS := -Wall -O2 -g

all: pma

pma: pma.cpp
	$(CXX) pma.cpp -o pma $(CXXFLAGS)

clean:
	rm -f pma
