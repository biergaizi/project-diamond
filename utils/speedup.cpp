#include <string.h>
#include <getopt.h>

#include <cstdio>
#include <cstdint>
#include <format>
#include <stdexcept>

#define TILING_NO_HACK
#include "tiling.hpp"
using namespace Tiling;

int main(int argc, char** argv);
void parseArgs(int argc, char** argv);
Plan3D makePlan(size_t tileHalfTs);
size_t simulate(Plan3D plan);

std::array<size_t, 3> gridSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<size_t, 3> tileSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<char, 3>   tileType = {'-', '-', '-'};
size_t tileHalfTs = SIZE_MAX;
size_t timesteps = 1000;
bool parallelogramSlidingWindow = false;

int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	size_t numBatches = timesteps * 2 / tileHalfTs;
	size_t remHalfTs = (timesteps - (numBatches * tileHalfTs) / 2) * 2;

	fprintf(stderr, "grid\t\t" "%04zu x %04zu x %04zu\n",
					gridSize[0], gridSize[1], gridSize[2]);
	fprintf(stderr, "tile\t\t" "%04zu x %04zu x %04zu\n",
					tileSize[0], tileSize[1], tileSize[2]);

	fprintf(stderr, "timesteps\t"  "%zu\n", timesteps);
	fprintf(stderr, "main batch\t" "%04zu x %04zu = %04zu timesteps\n",
					tileHalfTs / 2, numBatches, (numBatches * tileHalfTs) / 2);

	if (remHalfTs > 0) {
		fprintf(stderr, "rem batch\t" "%04zu x 0001 = %04zu timesteps\n",
						remHalfTs / 2, remHalfTs / 2);
	}
	else {
		fprintf(stderr, "rem batch\t" "0000 x 0000 = 0000 timesteps\n");
	}

	Plan3D mainPlan = makePlan(tileHalfTs);
	size_t totalBytesTransferred = simulate(mainPlan) * numBatches;

	if (remHalfTs > 0) {
		Plan3D remPlan = makePlan(remHalfTs);
		totalBytesTransferred += simulate(remPlan);
	}
	
	size_t naiveBytesTransferred = gridSize[0] * gridSize[1] * gridSize[2];
	naiveBytesTransferred *= 3;  // vec3
	naiveBytesTransferred *= 4;  // sizeof(float)
	naiveBytesTransferred *= 10; // volt r/w, curr r, vv r, vi r
								 // curr r/w, volt r, ii r, iv r
	naiveBytesTransferred *= timesteps;

	printf("tiled total\t" "%.0f MBytes\n", totalBytesTransferred / 1e6);
	printf("naive total\t" "%.0f MBytes\n", naiveBytesTransferred / 1e6);
	
	double speedup = 100.0 * naiveBytesTransferred / totalBytesTransferred;
	printf("speedup\t\t" "%.1f%%\n", speedup);
}

void parseArgs(int argc, char** argv)
{
	static struct option longopts[] = {
		{"sliding-window",		no_argument,       0, 'w'},
		{"grid-size",			required_argument, 0, 'g'},
		{"tile-size",			required_argument, 0, 't'},
		{"tile-height",			required_argument, 0, 'h'},
		{"total-timesteps",		optional_argument, 0, 'n'},
	};

	const char* progname = "speedup";
	if (argc > 0) {
		// argc == 0 is possible. The author is pedantic enough to worry about
		// such a theoretical security exploit in demo code...
		progname = argv[0];
	}

	char* gridArg = NULL;
	char* tileArg = NULL;
	int opt;

	while ((opt = getopt_long(argc, argv, "wg:t:h:n:", longopts, NULL)) != -1) {
		switch (opt) {
			case 'g':
				gridArg = optarg;
				break;
			case 't':
				tileArg = optarg;
				break;
			case 'h':
				tileHalfTs = atoi(optarg);
				break;
			case 'w':
				parallelogramSlidingWindow = true;
				break;
			default:
				break;
		}
	}

	if (!gridArg || !tileArg || tileHalfTs == SIZE_MAX) {
		printf("%s: Calculate theoretical DRAM traffic saving.\n\n", progname);
		printf("Usage: %s [OPTION]\n", progname);
		printf("   --grid-size\t\t-g\ti,j,k\t\t\t(e.g: 400,400,400)\n");
		printf("   --tile-size\t\t-t\tit,jt,kt/kp\t\t"
			   "(e.g: 20t,20t,20t or 20t,20t,20p)\n");
		printf("   --tile-height\t-h\thalfTimesteps\t\t(e.g: 18)\n");
		printf("   --total-timesteps\t-n\ttimesteps\t\t(defafult: 1000)\n");
		printf("   --sliding-window\t-w\tuse parallelogram sliding"
			                                         "\t(default: no)\n");
		printf("\nNote: Parallelogram tiling uses suffix \"p\", "
			   "trapezoid tiling uses suffix \"t\".\n");
		printf("Note: It assumes ideal data access patterns and infinitely-fast "
			   "code and cache - actual speedup is much lower.\n");
		std::exit(1);
	}

	gridSize[0] = atoi(strtok(gridArg, ","));
	gridSize[1] = atoi(strtok(NULL, ","));
	gridSize[2] = atoi(strtok(NULL, ","));

	std::array<std::string, 3> tileArgString;
	tileArgString[0] = strtok(tileArg, ",");
	tileArgString[1] = strtok(NULL, ",");
	tileArgString[2] = strtok(NULL, ",");

	for (size_t dim = 0; dim < 3; dim++) {
		std::string& arg = tileArgString[dim];

		if (arg[arg.size() - 1] != 't' && arg[arg.size() - 1] != 'p') {
			throw std::invalid_argument(
				std::format("tile suffix must be 't' or 'p', got {}",
							arg[arg.size() - 1])
			);
		}

		tileType[dim] = arg[arg.size() - 1];
		arg[arg.size() - 1] = '\0';
		tileSize[dim] = atoi(arg.c_str());
	}

	if (tileType[0] != 't' || tileType[1] != 't') {
		throw std::invalid_argument(
			"dimension i and j only support trapezoid tiling (suffix t)"
		);
	}
	if (tileType[2] == 't' && parallelogramSlidingWindow) {
		throw std::invalid_argument(
			"dimension k uses trapezoid tilling, "
			"parallelogram sliding window is unsupported."
		);
	}
}

Plan3D makePlan(size_t tileHalfTs)
{
	Plan1D i = computeTrapezoidTiles(gridSize[0], tileSize[0], tileHalfTs);
	Plan1D j = computeTrapezoidTiles(gridSize[1], tileSize[1], tileHalfTs);

	if (tileType[2] == 'p') {
		Plan1D k = computeParallelogramTiles(
			gridSize[2], tileSize[2], tileHalfTs
		);
		Plan3D plan = combineTilesTTP(i, j, k);
		return plan;
	}
	else if (tileType[2] == 't') {
		Plan1D k = computeTrapezoidTiles(
			gridSize[2], tileSize[2], tileHalfTs
		);
		Plan3D plan = combineTilesTTT(i, j, k);
		return plan;
	}
	else {
		throw std::invalid_argument(
			std::format("tile suffix must be 't' or 'p', got {}",
						tileType[2])
		);
	}
}

size_t simulate(Plan3D plan)
{
	size_t totalBytesTransferred = 0;

	size_t stage = 0;
	for (const TileList3D& tileList : plan) {
		for (const Tile3D& tile : tileList) {
			size_t subtileId = 0;

			for (const Subtile3D& subtile : tile) {
				size_t i = subtile.last[0] - subtile.first[0];
				size_t j = subtile.last[1] - subtile.first[1];

				size_t k;
				if (subtileId == 0 || !parallelogramSlidingWindow) {
					k = subtile.last[2] - subtile.first[2];
				}
				else {
					size_t lastK = tile[subtileId - 1].last[2];
					size_t currK = subtile.last[2];
					k = currK - lastK;
				}

				size_t tileBytesTransferred = i * j * k;
				tileBytesTransferred *= 3;  // vec3
				tileBytesTransferred *= 4;  // sizeof(float)
				tileBytesTransferred *= 8;  // volt r/w, curr r/w,
											// vv r, vi r, ii r, iv r
				totalBytesTransferred += tileBytesTransferred;

				subtileId++;
			}
		}
		stage++;
	}

	return totalBytesTransferred;
}
