CXX = g++
CXXFLAGS = -O3 -march=native -pipe -std=c++20 -pedantic -Wall -Wextra -Wno-vla

all: verify verify-simd

tiling.o: ../tiling/tiling.cpp ../tiling/tiling.hpp
	$(CXX) $(CXXFLAGS) -c ../tiling/tiling.cpp -o tiling.o

kernel-scalar.o: kernel-scalar.hpp kernel-scalar.cpp narray3d.hpp
	$(CXX) $(CXXFLAGS) -c kernel-scalar.cpp -o kernel-scalar.o

kernel-simd.o: kernel-simd.hpp kernel-simd.cpp narray3d.hpp simd.hpp
	$(CXX) $(CXXFLAGS) -c kernel-simd.cpp -o kernel-simd.o

verify: verify.cpp kernel-scalar.o tiling.o
	$(CXX) $(CXXFLAGS) -c verify.cpp -o verify.o -I../tiling
	$(CXX) $(CXXFLAGS) verify.o kernel-scalar.o tiling.o -o verify -lginac

verify-simd: verify-simd.cpp kernel-scalar.o kernel-simd.o tiling.o simd.hpp
	$(CXX) $(CXXFLAGS) -c verify-simd.cpp -o verify-simd.o -I../tiling
	$(CXX) $(CXXFLAGS) verify-simd.o kernel-scalar.o kernel-simd.o \
	                   tiling.o -o verify-simd -lginac

clean:
	rm -f *.o verify verify-simd
