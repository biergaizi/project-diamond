CXX=g++
CXXFLAGS=-Og -g -ggdb -march=native -Wall -Wextra -Wno-sign-compare
#CXXFLAGS=-O3 -march=native -Wall -Wextra

kernel-sym.o: kernel-sym.cpp
	$(CXX) $(CXXFLAGS) -c kernel-sym.cpp -o kernel-sym.o

tiling.o: tiling.h tiling.cpp
	$(CXX) $(CXXFLAGS) -c tiling.cpp -o tiling.o
	
verify.elf: verify.cpp kernel-sym.o tiling.o
	$(CXX) $(CXXFLAGS) -c verify.cpp -o verify.o
	$(CXX) $(CXXFLAGS) verify.o kernel-sym.o tiling.o -o verify.elf -lginac

clean:
	rm -rf *.o *.elf
