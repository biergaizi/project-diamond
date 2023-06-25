#include <cstdlib>
#include <ginac/ginac.h>
#include "array-nxyz.h"
#include "kernel-sym.h"
#include "tiling.h"

void initializeSymbolicArrays(
	const unsigned int numLines[3],
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii
)
{
	/* initialize arrays with expressions or symbols */
	for (unsigned int n = 0; n < 3; n++) {
		for (unsigned int x = 0; x < numLines[0]; x++) {
			for (unsigned int y = 0; y < numLines[1]; y++) {
				for (unsigned int z = 0; z < numLines[2]; z++) {
					char buf[20];
					volt.array.push_back(GiNaC::ex());
					curr.array.push_back(GiNaC::ex());

					snprintf(buf, 20, "vv(%d,%d,%d,%d)", n, x, y, z);
					vv.array.push_back(GiNaC::symbol(buf));

					snprintf(buf, 20, "vi(%d,%d,%d,%d)", n, x, y, z);
					vi.array.push_back(GiNaC::symbol(buf));

					snprintf(buf, 20, "iv(%d,%d,%d,%d)", n, x, y, z);
					iv.array.push_back(GiNaC::symbol(buf));

					snprintf(buf, 20, "ii(%d,%d,%d,%d)", n, x, y, z);
					ii.array.push_back(GiNaC::symbol(buf));
				}
			}
		}
	}

	/* assign initial values to volt and curr */
	for (unsigned int n = 0; n < 3; n++) {
		for (unsigned int x = 0; x < numLines[0]; x++) {
			for (unsigned int y = 0; y < numLines[1]; y++) {
				for (unsigned int z = 0; z < numLines[2]; z++) {
					char buf[20];

					snprintf(buf, 20, "volt(%d,%d,%d,%d)", n, x, y, z);
					volt(n, x, y, z) = GiNaC::symbol(buf);

					snprintf(buf, 20, "curr(%d,%d,%d,%d)", n, x, y, z);
					curr(n, x, y, z) = GiNaC::symbol(buf);
				}
			}
		}
	}
}


void copyFields(
	const unsigned int numLines[3],
	N_3DArray<GiNaC::ex>& volt_src,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr_src,
	N_3DArray<GiNaC::ex>& curr
)
{
	for (unsigned int n = 0; n < 3; n++) {
		for (unsigned int x = 0; x < numLines[0]; x++) {
			for (unsigned int y = 0; y < numLines[1]; y++) {
				for (unsigned int z = 0; z < numLines[2]; z++) {
					volt.array.push_back(volt_src(n, x, y, z));
					curr.array.push_back(curr_src(n, x, y, z));
				}
			}
		}
	}
}


void compareResults(
	const unsigned int numLines[3],
	N_3DArray<GiNaC::ex>& volt_ref,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr_ref,
	N_3DArray<GiNaC::ex>& curr
)
{
	for (unsigned int x = 0; x < numLines[0]; x++) {
		for (unsigned int y = 0; y < numLines[1]; y++) {
			for (unsigned int z = 0; z < numLines[2]; z++) {
				for (unsigned int n = 0; n < 3; n++) {
					if (volt_ref(n, x, y, z) != volt(n, x, y, z)) {
						fprintf(stderr, "volt(%d,%d,%d,%d) verification failed!\n", n, x, y, z);
						std::cerr << "Expected: ";
						std::cerr << volt_ref(n, x, y, z);
						std::cerr << "\n";
						std::cerr << "Received: ";
						std::cerr << volt(n, x, y, z);
						std::cerr << "\n";
						std::exit(1);
					}
					if (curr_ref(n, x, y, z) != curr(n, x, y, z)) {
						fprintf(stderr, "curr(%d,%d,%d,%d) verification failed!\n", n, x, y, z);
						std::cerr << "Expected: ";
						std::cerr << curr_ref(n, x, y, z);
						std::cerr << "\n";
						std::cerr << "Received: ";
						std::cerr << curr(n, x, y, z);
						std::cerr << "\n";
						std::exit(1);
					}
				}
			}
		}
	}
}
	

void generateGoldenResult(
	const unsigned int numLines[3], int timesteps,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii
)
{
	size_t size = numLines[0] * numLines[1] * numLines[2];
	fprintf(stderr, "generating golden result of %ld cells for %d timesteps.\n", size, timesteps);


	for (int i = 0; i < timesteps; i++) {
		UpdateVoltages(
			volt, curr, vv, vi,
			0, numLines[0] - 1,
			0, numLines[1] - 1,
			0, numLines[2] - 1
		);
		UpdateCurrents(
			curr, volt, iv, ii,
			0, numLines[0] - 2,
			0, numLines[1] - 2,
			0, numLines[2] - 2
		);
		//fprintf(stderr, "timestep: %d\n", i);
	}
}

void testRectangularTiling(
	const unsigned int numLines[3], int blkSizes[3],
	int timesteps,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii
)
{
	int numThreads = 4;
	auto tiles = computeRectangularTiles3D((int*) numLines, blkSizes, numThreads);

	fprintf(stderr, "testing rectangular tiling with %d threads.\n", numThreads);

	for (int t = 0; t < timesteps; t++) {
		for (int thread = 0; thread < numThreads; thread++) {
			for (int stage = 0; stage < tiles.size(); stage++) {
				for (int tileID = 0; tileID < tiles[stage][thread].size(); tileID++) {
					auto tile = tiles[stage][thread][tileID];
					//fprintf(stderr, "Voltage (%d,%d) (%d,%d) (%d,%d) t %d\n",
					//	tile.voltageStart[0], tile.voltageStop[0],
					//	tile.voltageStart[1], tile.voltageStop[1],
					//	tile.voltageStart[2], tile.voltageStop[2],
					//	thread
					//);

					UpdateVoltages(
						volt, curr, vv, vi,
						tile.voltageStart[0], tile.voltageStop[0],
						tile.voltageStart[1], tile.voltageStop[1],
						tile.voltageStart[2], tile.voltageStop[2]
					);
				}
			}
		}

		for (int thread = 0; thread < numThreads; thread++) {
			for (int stage = 0; stage < tiles.size(); stage++) {
				for (int tileID = 0; tileID < tiles[stage][thread].size(); tileID++) {
					auto tile = tiles[stage][thread][tileID];
					//fprintf(stderr, "Current (%d,%d) (%d,%d) (%d,%d) t %d\n",
					//	tile.currentStart[0], tile.currentStop[0],
					//	tile.currentStart[1], tile.currentStop[1],
					//	tile.currentStart[2], tile.currentStop[2],
					//	thread
					//);
					UpdateCurrents(
						curr, volt, iv, ii,
						tile.currentStart[0], (tile.currentStop[0] > numLines[0] - 2) ? numLines[0] - 2 : tile.currentStop[0],
						tile.currentStart[1], (tile.currentStop[1] > numLines[1] - 2) ? numLines[1] - 2 : tile.currentStop[1],
						tile.currentStart[2], (tile.currentStop[2] > numLines[2] - 2) ? numLines[2] - 2 : tile.currentStop[2]
					);

				}
			}
		}
	}
}

void testTrapezoidalTiling(
	const unsigned int numLines[3], int blkSizes[3],
	int timesteps,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii
)
{
	fprintf(stderr, "testing trapezoidal tiling with %d timesteps tiling.\n", timesteps);

	Tiles tilesX = computeParallelogramTiles1D(numLines[0], blkSizes[0], 60 * 2);
	Tiles tilesY = computeParallelogramTiles1D(numLines[1], blkSizes[1], 60 * 2);
	Tiles tilesZ = computeParallelogramTiles1D(numLines[2], blkSizes[2], 60 * 2);
	Tiles3D tiles = combineTilesTo3D(tilesX, tilesY, tilesZ, 60 * 2, 1)[0][0];

	for (int t = 0; t < 1; t++) {
		for (auto& tile : tiles) {
			//fprintf(stderr, "UpdateVoltages (%d, %d) (%d, %d) (%d, %d)\n",
			//	tile.voltageStart[0], tile.voltageStop[0],
			//	tile.voltageStart[1], tile.voltageStop[1],
			//	tile.voltageStart[2], tile.voltageStop[2]
			//);
			UpdateVoltages(
				volt, curr, vv, vi,
				tile.voltageStart[0], tile.voltageStop[0],
				tile.voltageStart[1], tile.voltageStop[1],
				tile.voltageStart[2], tile.voltageStop[2]
			);

			//fprintf(stderr, "UpdateCurrents (%d, %d) (%d, %d) (%d, %d)\n",
			//	tile.currentStart[0], (tile.currentStop[0] > numLines[0] - 2) ? numLines[0] - 2 : tile.currentStop[0],
			//	tile.currentStart[1], (tile.currentStop[1] > numLines[1] - 2) ? numLines[1] - 2 : tile.currentStop[1],
			//	tile.currentStart[2], (tile.currentStop[2] > numLines[2] - 2) ? numLines[2] - 2 : tile.currentStop[2]
			//);
			UpdateCurrents(
				curr, volt, iv, ii,
				tile.currentStart[0], (tile.currentStop[0] > numLines[0] - 2) ? numLines[0] - 2 : tile.currentStop[0],
				tile.currentStart[1], (tile.currentStop[1] > numLines[1] - 2) ? numLines[1] - 2 : tile.currentStop[1],
				tile.currentStart[2], (tile.currentStop[2] > numLines[2] - 2) ? numLines[2] - 2 : tile.currentStop[2]
			);
		}
	}
}

void testDiamondTiling(
	const unsigned int numLines[3], int blkSizes[3],
	int timesteps,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii
)
{
	fprintf(stderr, "testing diamond tiling with %d timesteps tiling.\n", timesteps);

	Tiles tilesX = computeDiamondTiles1D(numLines[0], blkSizes[0], timesteps * 2);
	Tiles tilesY = computeDiamondTiles1D(numLines[1], blkSizes[1], timesteps * 2);
	Tiles tilesZ = computeDiamondTiles1D(numLines[2], blkSizes[2], timesteps * 2);
	auto tilesPerPhasePerThread = combineTilesTo3D(tilesX, tilesY, tilesZ, timesteps * 2, 4);

	int totalPhases = tilesPerPhasePerThread[0].size();

	for (int t = 0; t < 1; t++) {
		for (int phase = 0; phase < totalPhases; phase++) {
			for (int threadID = 0; threadID < 4; threadID++) {
				/* shuffle thread to test out-of-order execution */
				int threads[4] = {2, 4, 3, 1};
				auto tiles = tilesPerPhasePerThread[threads[threadID] - 1][phase];

				for (auto& tile : tiles) {
					//fprintf(stderr, "UpdateVoltages (%d, %d) (%d, %d) (%d, %d)\n",
					//	tile.voltageStart[0], tile.voltageStop[0],
					//	tile.voltageStart[1], tile.voltageStop[1],
					//	tile.voltageStart[2], tile.voltageStop[2]
					//);
					UpdateVoltages(
						volt, curr, vv, vi,
						tile.voltageStart[0], tile.voltageStop[0],
						tile.voltageStart[1], tile.voltageStop[1],
						tile.voltageStart[2], tile.voltageStop[2]
					);

					//fprintf(stderr, "UpdateCurrents (%d, %d) (%d, %d) (%d, %d)\n",
					//	tile.currentStart[0], (tile.currentStop[0] > numLines[0] - 2) ? numLines[0] - 2 : tile.currentStop[0],
					//	tile.currentStart[1], (tile.currentStop[1] > numLines[1] - 2) ? numLines[1] - 2 : tile.currentStop[1],
					//	tile.currentStart[2], (tile.currentStop[2] > numLines[2] - 2) ? numLines[2] - 2 : tile.currentStop[2]
					//);
					UpdateCurrents(
						curr, volt, iv, ii,
						tile.currentStart[0], (tile.currentStop[0] > numLines[0] - 2) ? numLines[0] - 2 : tile.currentStop[0],
						tile.currentStart[1], (tile.currentStop[1] > numLines[1] - 2) ? numLines[1] - 2 : tile.currentStop[1],
						tile.currentStart[2], (tile.currentStop[2] > numLines[2] - 2) ? numLines[2] - 2 : tile.currentStop[2]
					);
				}
			}
		}
	}
}

int main(void)
{
	const unsigned int numLines[3] = {20, 20, 20};
	int blkSizes[3] = {4, 4, 4};
	int timesteps = 60;

	/* reference result */
	N_3DArray<GiNaC::ex>& volt_golden = *Create_N_3DArray<GiNaC::ex>(numLines);
	N_3DArray<GiNaC::ex>& curr_golden = *Create_N_3DArray<GiNaC::ex>(numLines);
	N_3DArray<GiNaC::symbol>& vv = *Create_N_3DArray<GiNaC::symbol>(numLines);
	N_3DArray<GiNaC::symbol>& vi = *Create_N_3DArray<GiNaC::symbol>(numLines);
	N_3DArray<GiNaC::symbol>& iv = *Create_N_3DArray<GiNaC::symbol>(numLines);
	N_3DArray<GiNaC::symbol>& ii = *Create_N_3DArray<GiNaC::symbol>(numLines);

	/* result under test */
	N_3DArray<GiNaC::ex>& volt = *Create_N_3DArray<GiNaC::ex>(numLines);
	N_3DArray<GiNaC::ex>& curr = *Create_N_3DArray<GiNaC::ex>(numLines);

	initializeSymbolicArrays(numLines, volt_golden, curr_golden, vv, vi, iv, ii);
	copyFields(numLines, volt_golden, volt, curr_golden, curr);

	generateGoldenResult(numLines, timesteps, volt_golden, curr_golden, vv, vi, iv, ii);
	//testRectangularTiling(numLines, blkSizes, timesteps, volt, curr, vv, vi, iv, ii);
	//testTrapezoidalTiling(numLines, blkSizes, timesteps, volt, curr, vv, vi, iv, ii);
	testDiamondTiling(numLines, blkSizes, timesteps, volt, curr, vv, vi, iv, ii);

	compareResults(numLines, volt_golden, volt, curr_golden, curr);
	fprintf(stderr, "verification passed.\n");

	return 0;
}
