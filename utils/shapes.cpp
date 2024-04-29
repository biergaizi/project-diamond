#include <string.h>
#include <getopt.h>

#include <cstdio>
#include <cstdint>
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <format>

#define TILING_NO_HACK
#include "tiling.hpp"
using namespace Tiling;

int main(int argc, char** argv);
void parseArgs(int argc, char** argv);
Plan3D makePlan(size_t tileHalfTs);

std::array<size_t, 3> gridSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<size_t, 3> tileSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<char, 3>   tileType = {'-', '-', '-'};
size_t tileHalfTs = SIZE_MAX;

using SubtileSizeKey = std::array<size_t, 3>;

struct SubtileSizeKeyHash {
	size_t operator()(const SubtileSizeKey& key) const
	{
		if (key[0] > UINT16_MAX || key[1] > UINT16_MAX || key[2] > UINT16_MAX) {
			throw std::runtime_error("i/j/k cannot be greater than 65536");
		}

		size_t hash = key[0] + (key[1] << 16) + (key[2] << 32);
		return hash;
	}
};

using SubtileSizeMap = std::unordered_map<
	SubtileSizeKey, size_t, SubtileSizeKeyHash
>;

int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	printf("grid\t\t" "%04zu x %04zu x %04zu\n",
		   gridSize[0], gridSize[1], gridSize[2]);
	printf("tile\t\t" "%04zu x %04zu x %04zu\n",
		   tileSize[0], tileSize[1], tileSize[2]);

	Plan3D plan = makePlan(tileHalfTs);
	
	SubtileSizeMap map;
	size_t stage = 0;
	for (const TileList3D& tileList : plan) {
		for (const Tile3D& tile : tileList) {
			for (const Subtile3D& subtile : tile) {
				std::array<size_t, 3> size = {
					subtile.last[0] - subtile.first[0] + 1,
					subtile.last[1] - subtile.first[1] + 1,
					subtile.last[2] - subtile.first[2] + 1
				};

				if (map.find(size) == map.end()) {
					map[size] = 1;
				}
				else {
					map[size]++;
				}
			}
		}
		stage++;
	}

	size_t totalOverlappedBytes = 0;
	printf("\n%zu unique subtile shapes found.\n", map.size());
	for (auto& [key, val]: map) {
		printf("%02zu x %02zu x %02zu\t" "%zu\tsubtiles\n",
			   key[0], key[1], key[2], val);

		size_t bytes = key[0] * key[1] * key[2];
		bytes *= 3;  // vec3
		bytes *= 4;  // sizeof(float)
		bytes *= 4;  // vv, vi, iv, ii
		bytes *= val;

		totalOverlappedBytes += bytes;
	}

	size_t totalNaiveBytes = gridSize[0] * gridSize[1] * gridSize[2];
	totalNaiveBytes *= 3;
	totalNaiveBytes *= 4;
	totalNaiveBytes *= 4;

	printf("%zu bytes of RAM needed if grid is stored naively\n"
		   "%zu bytes of RAM needed if overlapped tiles are stored "
		   "multiple times\n", totalNaiveBytes, totalOverlappedBytes);
}

void parseArgs(int argc, char** argv)
{
	static struct option longopts[] = {
		{"grid-size",			required_argument, 0, 'g'},
		{"tile-size",			required_argument, 0, 't'},
		{"tile-height",			required_argument, 0, 'h'},
	};

	const char* progname = "shapes";
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
			default:
				break;
		}
	}

	if (!gridArg || !tileArg || tileHalfTs == SIZE_MAX) {
		printf("%s: Show statistics of all unique tile shapes.\n\n", progname);
		printf("Usage: %s [OPTION]\n", progname);
		printf("   --grid-size\t\t-g\ti,j,k\t\t\t(e.g: 400,400,400)\n");
		printf("   --tile-size\t\t-t\tit,jt,kt/kp\t\t"
			   "(e.g: 20t,20t,20t or 20t,20t,20p)\n");
		printf("   --tile-height\t-h\thalfTimesteps\t\t(e.g: 18)\n");
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
