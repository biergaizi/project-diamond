CXX = g++
CXXFLAGS = -O3 -march=native -pipe -std=c++20 -pedantic -Wall -Wextra -Wno-vla

all: sanity

tiling.o: ../tiling/tiling.cpp ../tiling/tiling.hpp
	$(CXX) $(CXXFLAGS) -c ../tiling/tiling.cpp -o tiling.o

kernel.o: ../tiling/tiling.cpp kernel.cpp kernel.hpp array3d.hpp
	$(CXX) $(CXXFLAGS) -c kernel.cpp -o kernel.o -I../tiling/

sanity: sanity.cpp array3d.hpp tiling.o kernel.o
	$(CXX) $(CXXFLAGS) -c sanity.cpp -o sanity.o -I../tiling/
	$(CXX) $(CXXFLAGS) tiling.o sanity.o kernel.o -o sanity

clean:
	rm -f *.o sanity
