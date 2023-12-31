#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>

typedef std::pair<int, int>	Range;
typedef std::vector<Range>	Block;

enum {
	TILES_RECTANGULAR,
	TILES_PARALLELOGRAM,
	TILES_DIAMOND,
};

struct Tiles {
	/* TILES_PARALLELOGRAM or TILES_DIAMOND */
	int type;

	/*
	 * TILES_PARALLELOGRAM has one phases,
	 * TILES_DIAMOND has two.
	 */
	int phases;

	/* 
	 * First index: phase.
	 * TILES_PARALLELOGRAM has one phase.
	 * TILES_DIAMOND has mountain and valley phases.
	 *
	 * Second index: block.
	 * Depending on the tiling parameters, the width
	 * of a block varies.
	 *
	 * Third index: timestep.
	 * Depending on the tiling parameters, the number
	 * of timesteps within one block varies.
	 */
	std::vector<std::vector<Block>> array;
};

Tiles computeParallelogramTiles1D(
	int totalWidth,
	int blkWidth,
	int blkTimesteps
);
Tiles computeDiamondTiles1D(
	int totalWidth,
	int blkWidth,
	int blkTimesteps
);

struct Range3D
{
	int timestep;
	int voltageStart[3];
	int voltageStop[3];
	int currentStart[3];
	int currentStop[3];
};
typedef std::vector<Range3D>	Tiles3D;

Tiles computeRectangularTilesNoDeps1D(int totalWidth, int blkWidth, int blkHalfTimesteps);
Tiles computeRectangularTiles1D(int totalWidth, int blkWidth, int blkHalfTimesteps);
std::vector<std::vector<Tiles3D>> computeRectangularTiles3D(
	int totalWidth[3],
	int blkWidth[3],
	int numThreads
);
Tiles3D combineTilesTo3D(Tiles tilesX, Tiles tilesY, Tiles tilesZ, int blkHalfTimesteps);
void visualizeTiles(Tiles tiles, int totalWidth, int blkTimesteps);
std::vector<std::vector<Tiles3D>> combineTilesTo3D(
	Tiles tilesX, Tiles tilesY, Tiles tilesZ,
	int blkHalfTimesteps,
	int numThreads
);
