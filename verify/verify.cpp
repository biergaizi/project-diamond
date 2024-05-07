#include <getopt.h>
#include <format>
#include "kernel.hpp"

#include "tiling.hpp"
using namespace Tiling;

std::array<size_t, 3> gridSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<size_t, 3> tileSize = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
std::array<char, 3>   tileType = {'-', '-', '-'};
size_t tileHalfTs = SIZE_MAX;
size_t timesteps = SIZE_MAX;
bool debug = false;

void parseArgs(int argc, char** argv);

void initializeSymbolicArrays(const char* name, NArray3D<GiNaC::ex>& array);

void copySymbolicArrays(
	NArray3D<GiNaC::ex>& arrayDst,
	NArray3D<GiNaC::ex>& arraySrc
);

bool compareSymbolicArrays(
	NArray3D<GiNaC::ex>& arrayRef,
	NArray3D<GiNaC::ex>& arrayTiled
);

int main(int argc, char** argv);

void ref(
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
);

Plan3D makePlan(size_t tileHalfTs);

void tiled(
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
);

void tiledBody(
	Plan3D plan,
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
);

void parseArgs(int argc, char** argv)
{
	static struct option longopts[] = {
		{"grid-size",			required_argument, 0, 'g'},
		{"tile-size",			required_argument, 0, 't'},
		{"tile-height",			required_argument, 0, 'h'},
		{"total-timesteps",		optional_argument, 0, 'n'},
	};

	const char* progname = "verify";
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
		printf("%s: Symbolic Verification of Tiling Correctness\n\n", progname);
		printf("Usage: %s [OPTION]\n", progname);
		printf("   --grid-size\t\t-g\ti,j,k\t\t\t(e.g: 400,400,400)\n");
		printf("   --tile-size\t\t-t\tit,jt,kt/kp\t\t"
			   "(e.g: 20t,20t,20t or 20t,20t,20p)\n");
		printf("   --tile-height\t-h\thalfTimesteps\t\t(e.g: 18)\n");
		printf("   --total-timesteps\t-n\ttimesteps\t\t(defafult: 100)\n");
		printf("   --dump\t\t-d\tdump traces for debugging\t(default: no)\n");
		printf("\nNote: Parallelogram tiling uses suffix \"p\", "
			   "trapezoid tiling uses suffix \"t\".\n");
		printf("Note: Symbolic verification requires extreme memory usage. "
			   "64 GiB PC is\nrequired for a 70,70,70 grid with timestep "
			   "size of 20, don't even think\nabout trying more timesteps "
			   "unless more memory is available.\n");
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

void initializeSymbolicArrays(NArray3D<GiNaC::ex>& array)
{
	for (size_t n = 0; n < 3; n++) {
		for (size_t i = 0; i < gridSize[0]; i++) {
			for (size_t j = 0; j < gridSize[1]; j++) {
				for (size_t k = 0; k < gridSize[2]; k++) {
					auto symbolName = std::format(
						"{}({},{},{},{})", array.name(), i, j, k, n
					);

					array(i, j, k, n) = GiNaC::symbol(symbolName.c_str());
				}
			}
		}
	}
}

void copySymbolicArrays(
	NArray3D<GiNaC::ex>& arrayDst,
	NArray3D<GiNaC::ex>& arraySrc
)
{
	for (size_t n = 0; n < 3; n++) {
		for (size_t i = 0; i < gridSize[0]; i++) {
			for (size_t j = 0; j < gridSize[1]; j++) {
				for (size_t k = 0; k < gridSize[2]; k++) {
					arrayDst(i, j, k, n) = arraySrc(i, j, k, n);
				}
			}
		}
	}
}

bool compareSymbolicArrays(
	NArray3D<GiNaC::ex>& arrayRef,
	NArray3D<GiNaC::ex>& arrayTiled
)
{
	for (size_t n = 0; n < 3; n++) {
		for (size_t i = 0; i < gridSize[0]; i++) {
			for (size_t j = 0; j < gridSize[1]; j++) {
				for (size_t k = 0; k < gridSize[2]; k++) {
					if (arrayTiled(i, j, k, n) != arrayRef(i, j, k, n)) {
						std::stringstream arrayTiledString, arrayRefString;

						arrayTiledString << arrayTiled(i, j, k, n);
						arrayRefString << arrayRef(i, j, k, n);

						std::cerr << std::format(
							"{}(i={},j={},k={},n={}) verification failed!\n\n"
							"Expected:\n\n{}\n\n"
							"Received:\n\n{}\n",
							arrayTiled.name().c_str(), i, j, k, n, 
							arrayRefString.str(),
							arrayTiledString.str()
						);
						return false;
					}
				}
			}
		}
	}
	return true;
}

int main(int argc, char** argv)
{
	parseArgs(argc, argv);

	printf("grid\t\t" "%04zu x %04zu x %04zu\n",
		   gridSize[0], gridSize[1], gridSize[2]);
	printf("tile\t\t" "%04zu x %04zu x %04zu\n",
		   tileSize[0], tileSize[1], tileSize[2]);
	printf("timesteps\t"  "%zu\n", timesteps);

	// common operator arrays, expected to be read-only
	auto vv = NArray3D<GiNaC::ex>("vv", gridSize);
	auto vi = NArray3D<GiNaC::ex>("vi", gridSize);
	auto ii = NArray3D<GiNaC::ex>("ii", gridSize);
	auto iv = NArray3D<GiNaC::ex>("iv", gridSize);

	initializeSymbolicArrays(vv);
	initializeSymbolicArrays(vi);
	initializeSymbolicArrays(ii);
	initializeSymbolicArrays(iv);

	// reference field and tiled field arrays
	auto voltRef = NArray3D<GiNaC::ex>("volt", gridSize);
	auto currRef = NArray3D<GiNaC::ex>("curr", gridSize);
	initializeSymbolicArrays(voltRef);
	initializeSymbolicArrays(currRef);

	auto voltTiled = NArray3D<GiNaC::ex>("volt", gridSize);
	auto currTiled = NArray3D<GiNaC::ex>("curr", gridSize);
	copySymbolicArrays(voltTiled, voltRef);
	copySymbolicArrays(currTiled, currRef);

	// calculate reference and tiled values
	ref(voltRef, currRef, vv, vi, ii, iv);
	tiled(voltTiled, currTiled, vv, vi, ii, iv);

	// then compare
	bool success = true;
	success &= compareSymbolicArrays(voltRef, voltTiled);
	success &= compareSymbolicArrays(currRef, currTiled);

	if (success) {
		printf("verification passed.\n");
	}

	return !success;
}

void ref(
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
)
{
	std::cout << "generating golden results...\n";

	for (size_t t = 0; t < timesteps; t++) {
		std::array<size_t, 3> rangeFirst = {0, 0, 0};

		std::array<size_t, 3> voltRangeLast = {
			gridSize[0] - 1, gridSize[1] - 1, gridSize[2] - 1
		};

		std::array<size_t, 3> currRangeLast = {
			gridSize[0] - 2, gridSize[1] - 2, gridSize[2] - 2
		};

		updateVoltageRange(
			volt, curr, vv, vi,
			rangeFirst, voltRangeLast,
			debug
		);

		updateCurrentRange(
			curr, volt, ii, iv,
			rangeFirst, currRangeLast,
			debug
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

void tiled(
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
)
{
	std::cout << "generating tiled results...\n";

	size_t numBatches = timesteps * 2 / tileHalfTs;
	size_t remHalfTs = (timesteps - (numBatches * tileHalfTs) / 2) * 2;

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
	for (size_t batchId = 0; batchId < numBatches; batchId++) {
		tiledBody(mainPlan, volt, curr, vv, vi, ii, iv);
	}

	if (remHalfTs > 0) {
		Plan3D remPlan = makePlan(remHalfTs);
		tiledBody(remPlan, volt, curr, vv, vi, ii, iv);
	}
}

void tiledBody(
	Plan3D plan,
	NArray3D<GiNaC::ex>& volt,
	NArray3D<GiNaC::ex>& curr,
	NArray3D<GiNaC::ex>& vv,
	NArray3D<GiNaC::ex>& vi,
	NArray3D<GiNaC::ex>& ii,
	NArray3D<GiNaC::ex>& iv
)
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

					updateVoltageRange(
						volt, curr, vv, vi,
						voltRange.first, voltRange.last,
						debug
					);

					updateCurrentRange(
						curr, volt, ii, iv,
						currRange.first, currRange.last,
						debug
					);
				}
			}
		}
		stage++;
	}
}
