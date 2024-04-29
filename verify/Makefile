CXX = g++
CXXFLAGS = -O3 -march=native -pipe -std=c++20 -pedantic -Wall -Wextra -Wno-vla

all: verify

tiling.o: ../tiling/tiling.cpp ../tiling/tiling.hpp
	$(CXX) $(CXXFLAGS) -c ../tiling/tiling.cpp -o tiling.o

kernel: kernel.hpp kernel.cpp narray3d.hpp
	$(CXX) $(CXXFLAGS) -c kernel.cpp -o kernel.o

verify: verify.cpp kernel.o tiling.o
	$(CXX) $(CXXFLAGS) -c verify.cpp -o verify.o -I../tiling
	$(CXX) $(CXXFLAGS) verify.o kernel.o tiling.o -o verify -lginac

clean:
	rm -f *.o verify
