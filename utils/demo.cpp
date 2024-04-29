#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <format>
#include <getopt.h>

#include "tiling.hpp"
using namespace Tiling;

std::array<size_t, 3> gridSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<size_t, 3> tileSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<char, 3>   tileType = {'-', '-', '-'};
size_t tileHalfTs = SIZE_MAX;
bool dumpRanges = false;

int main(int argc, char** argv);
void dumpAllTiles(Plan3D plan);
void parseArgs(int argc, char** argv);
Plan3D makePlan(size_t tileHalfTs);

int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	Plan3D plan = makePlan(tileHalfTs);	

	if (dumpRanges) {
		dumpAllTiles(plan);
	}

	return 0;
}

void dumpAllTiles(Plan3D plan)
{
	size_t stage = 0;
	for (const TileList3D& tileList : plan) {
		printf("\n***********stage: %zu****************\n", stage);
		for (const Tile3D& tile : tileList) {
			printf("\t=============tileId: (%zu, %zu, %zu)=============\n",
			       tile.id()[0], tile.id()[1], tile.id()[2]);
			for (const Subtile3D& subtile : tile) {
				printf("\t\t---(%zu, %zu, %zu) - (%zu, %zu, %zu)---\n",
					   subtile.first[0], subtile.first[1], subtile.first[2],
					   subtile.last[0],  subtile.last[1],  subtile.last[2]);
				for (const Range3D<size_t>& range : subtile) {
					printf("\t\t\t(%zu, %zu, %zu) - (%zu, %zu, %zu)\n",
						   range.first[0], range.first[1], range.first[2],
						   range.last[0],  range.last[1],  range.last[2]
					);
				}
				printf("\t\t---------------------------\n");
			}
			printf("\t===========================\n");
		}
		printf("***************************\n");
		stage++;
	}
}

void parseArgs(int argc, char** argv)
{
	static struct option opts[] = {
		{"grid-size",			required_argument, 0, 'g'},
		{"tile-size",			required_argument, 0, 't'},
		{"tile-height",			required_argument, 0, 'h'},
		{"dump",				optional_argument, 0, 'd'},
	};

	const char* progname = "demo";
	if (argc > 0) {
		// argc == 0 is possible. The author is pedantic enough to worry about
		// such a theoretical security exploit in demo code...
		progname = argv[0];
	}

	char* gridArg = NULL;
	char* tileArg = NULL;
	int opt;

	while ((opt = getopt_long(argc, argv, "dwg:t:h:n:", opts, NULL)) != -1) {
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
			case 'd':
				dumpRanges = true;
				break;
			default:
				break;
		}
	}

	if (!gridArg || !tileArg || tileHalfTs == SIZE_MAX) {
		printf("%s: Visualize how the simulation box is tiled "
			   "in ASCII diagram\n\n", progname);
		printf("Usage: %s [OPTION]\n", progname);
		printf("   --grid-size\t\t-g\ti,j,k\t\t\t(e.g: 100,100,100)\n");
		printf("   --tile-size\t\t-t\tit,jt,kt/kp\t\t"
			   "(e.g: 20t,20t,20t or 10t,10t,10p)\n");
		printf("   --tile-height\t-h\thalfTimesteps\t\t(e.g: 18)\n");
		printf("   --dump\t\t-d\tdump plan for debugging\t(default: no)\n");
		printf("\nNote: Parallelogram tiling uses suffix \"p\", "
			   "trapezoid tiling uses suffix \"t\".\n");
		printf("Note: Make sure the grid size is not too large, otherwise "
		       "the ASCII diagram won't fit in your terminal window.\n");
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
}

Plan3D makePlan(size_t tileHalfTs)
{
	Plan1D i = computeTrapezoidTiles(gridSize[0], tileSize[0], tileHalfTs);
	Plan1D j = computeTrapezoidTiles(gridSize[1], tileSize[1], tileHalfTs);

	printf("tiling for dimension i:\n");
	visualizeTiles(i, gridSize[0], tileSize[0], tileHalfTs);

	printf("\ntiling for dimension j:\n");
	visualizeTiles(j, gridSize[0], tileSize[0], tileHalfTs);

	if (tileType[2] == 'p') {
		Plan1D k = computeParallelogramTiles(
			gridSize[2], tileSize[2], tileHalfTs
		);
		Plan3D plan = combineTilesTTP(i, j, k);

		printf("\ntiling for dimension k:\n");
		visualizeTiles(k, gridSize[2], tileSize[2], tileHalfTs);

		return plan;
	}
	else if (tileType[2] == 't') {
		Plan1D k = computeTrapezoidTiles(
			gridSize[2], tileSize[2], tileHalfTs
		);
		Plan3D plan = combineTilesTTT(i, j, k);

		printf("\ntiling for dimension k:\n");
		visualizeTiles(k, gridSize[2], tileSize[2], tileHalfTs);

		return plan;
	}
	else {
		throw std::invalid_argument(
			std::format("tile suffix must be 't' or 'p', got {}",
						tileType[2])
		);
	}
}
