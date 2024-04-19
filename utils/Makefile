CXX = g++
CXXFLAGS = -O3 -march=native -pipe -std=c++20 -pedantic -Wall -Wextra -Wno-vla

all: demo speedup shapes

tiling.o: ../tiling/tiling.cpp ../tiling/tiling.hpp
	$(CXX) $(CXXFLAGS) -c ../tiling/tiling.cpp -o tiling.o

demo: demo.cpp tiling.o
	$(CXX) $(CXXFLAGS) -c demo.cpp -o demo.o -I../tiling
	$(CXX) $(CXXFLAGS) tiling.o demo.o -o demo

speedup: speedup.cpp tiling.o
	$(CXX) $(CXXFLAGS) -c speedup.cpp -o speedup.o -I../tiling
	$(CXX) $(CXXFLAGS) tiling.o speedup.o -o speedup

shapes: shapes.cpp tiling.o
	$(CXX) $(CXXFLAGS) -c shapes.cpp -o shapes.o -I../tiling
	$(CXX) $(CXXFLAGS) tiling.o shapes.o -o shapes

clean:
	rm -f *.o demo speedup shapes
