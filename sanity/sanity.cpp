#include <getopt.h>
#include <cstring>
#include <iostream>
#include <format>
#include "array3d.hpp"
#include "kernel.hpp"

#define TILING_NO_HACK
#include "tiling.hpp"
using namespace Tiling;

std::array<size_t, 3> gridSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<size_t, 3> tileSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<char, 3>   tileType = {'-', '-', '-'};
size_t tileHalfTs = SIZE_MAX;
size_t timesteps = SIZE_MAX;
bool debug = false;

void ref(void);
void tiled(void);
void tiledBody(Plan3D plan, Array3D<uint32_t>& volt, Array3D<uint32_t>& curr);
Plan3D makePlan(size_t tileHalfTs);
void parseArgs(int argc, char** argv);
int main(int argc, char** argv);

void ref(void)
{
	std::cout << "ref()...\n";

	auto volt = Array3D<uint32_t>(gridSize[0], gridSize[1], gridSize[2]);
	auto curr = Array3D<uint32_t>(gridSize[0], gridSize[1], gridSize[2]);

	for (size_t t = 0; t < timesteps; t++) {
		std::array<size_t, 3> rangeFirst = {0, 0, 0};

		std::array<size_t, 3> voltRangeLast = {
			gridSize[0] - 1, gridSize[1] - 1, gridSize[2] - 1
		};

		std::array<size_t, 3> currRangeLast = {
			gridSize[0] - 2, gridSize[1] - 2, gridSize[2] - 2
		};

		checkVoltageRange(volt, curr, rangeFirst, voltRangeLast, debug);
		checkCurrentRange(curr, volt, rangeFirst, currRangeLast, debug);
	}
	std::cout << "\tpassed!\n";
}

void tiled(void)
{
	std::cout << "tiled()...\n";

	size_t numBatches = timesteps * 2 / tileHalfTs;
	size_t remHalfTs = (timesteps - (numBatches * tileHalfTs) / 2) * 2;

	auto volt = Array3D<uint32_t>(gridSize[0], gridSize[1], gridSize[2]);
	auto curr = Array3D<uint32_t>(gridSize[0], gridSize[1], gridSize[2]);

	Plan3D mainPlan = makePlan(tileHalfTs);
	tiledBody(mainPlan, volt, curr);

	if (remHalfTs > 0) {
		Plan3D remPlan = makePlan(remHalfTs);
		tiledBody(remPlan, volt, curr);
	}

	std::cout << "\tpassed!\n";
}

void tiledBody(Plan3D plan, Array3D<uint32_t>& volt, Array3D<uint32_t>& curr)
{
	size_t stage = 0;
	for (const TileList3D& tileList : plan) {
		if (debug) {
			fprintf(stderr, "stage: %zu\n", stage);
		}
		for (const Tile3D& tile : tileList) {
			for (const Subtile3D& subtile : tile) {
				for (size_t halfTs = 0; halfTs < subtile.size(); halfTs += 2) {
					const Range3D<size_t>& voltRange = subtile[halfTs];
					const Range3D<size_t>& currRange = subtile[halfTs + 1];

					checkVoltageRange(
						volt, curr,
						voltRange.first, voltRange.last,
						debug
					);

					checkCurrentRange(
						curr, volt,
						currRange.first, currRange.last,
						debug
					);
				}
			}
		}
		stage++;
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

void parseArgs(int argc, char** argv)
{
	static struct option longopts[] = {
		{"grid-size",			required_argument, 0, 'g'},
		{"tile-size",			required_argument, 0, 't'},
		{"tile-height",			required_argument, 0, 'h'},
		{"total-timesteps",		optional_argument, 0, 'n'},
	};

	const char* progname = "sanity";
	if (argc > 0) {
		// argc == 0 is possible. The author is pedantic enough to worry about
		// such a theoretical security exploit in demo code...
		progname = argv[0];
	}

	char* gridArg = NULL;
	char* tileArg = NULL;
	int opt;

	while ((opt = getopt_long(argc, argv, "dg:t:h:n:", longopts, NULL)) != -1) {
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
			case 'n':
				timesteps = atoi(optarg);
				break;
			case 'd':
				debug = true;
				break;
			default:
				break;
		}
	}

	if (!gridArg || !tileArg ||
		tileHalfTs == SIZE_MAX || timesteps == SIZE_MAX
	) {
		printf("%s: Quick Sanity Check of Tiling Correctness\n\n", progname);
		printf("Usage: %s [OPTION]\n", progname);
		printf("   --grid-size\t\t-g\ti,j,k\t\t\t(e.g: 400,400,400)\n");
		printf("   --tile-size\t\t-t\tip,jp,kp/kt\t\t"
			   "(e.g: 20p,20p,20p or 20p,20p,20t)\n");
		printf("   --tile-height\t-h\thalfTimesteps\t\t(e.g: 18)\n");
		printf("   --total-timesteps\t-n\ttimesteps\t\t(defafult: 100)\n");
		printf("   --dump\t\t-d\tdump traces for debugging\t(default: no)\n");
		printf("\nNote: Parallelogram tiling uses suffix \"p\", "
			   "trapezoid tiling uses suffix \"t\".\n");
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

int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	tiled();
	ref();
}
