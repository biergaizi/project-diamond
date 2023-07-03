#include <cstdlib>
#include <algorithm>
#include "tiling.h"

/*
 * Split the simulation domain into regular rectangular tiles in 1D space.
 * No special time-skewing technique is used. This serves only as a reference
 * implementation.
 *
 * This implementation uses identical rectangular tile shapes and does not
 * consider dependencies between electric and magnetic field updates. Thus,
 * all electric (even) steps must be executed before all magnetic (old) steps.
 *
 * It's the caller's responsibility to check if the magnetic field tile is
 * at the edge of the simulation domain.
 */
Tiles computeRectangularTilesNoDeps1D(int totalWidth, int blkWidth, int blkHalfTimesteps)
{
	if (blkHalfTimesteps != 2)
	{
		/*
		 * One timestep contains two half timesteps, one electric, one
		 * magnetic. Multi-timestep techniques (time skewing) are not
		 * supported.
		 */
		std::exit(1);
	}

	std::vector<Block> blockList;

	/* at the beginning, divide the axis into several tiles */
	int numBlocks    = totalWidth / blkWidth;
	int numRemainder = totalWidth % blkWidth;

	/* For leftover cells, we allocate another block */
	if (numRemainder > 0) {
		numBlocks += 1;
	}

	blockList.resize(numBlocks);

	for (size_t i = 0; i < blockList.size(); i++) {
		Block block;

		for (int t = 0; t < 2; t++) {
			if (t % 2 == 0) {
				/* electric field range */
				Range startStop = Range(
					i * blkWidth,
					i * blkWidth + blkWidth - 1
				);
				if (startStop.second > totalWidth - 1) {
					startStop.second = totalWidth - 1;
				}
				block.push_back(startStop);
			}
			else {
				/* magnetic field range */
				Range startStop = Range(
					i * blkWidth,
					i * blkWidth + blkWidth - 1
				);
				if (startStop.second > totalWidth - 1) {
					startStop.second = totalWidth - 1;
				}
				block.push_back(startStop);
			}
		}

		blockList[i] = block;
	}

	/* TILES_RECTANGULAR has one phase. */
	Tiles tiles;
	tiles.type = TILES_RECTANGULAR;
	tiles.phases = 1;
	tiles.array.resize(tiles.phases);
	tiles.array[0] = blockList;

	return tiles;
}

/*
 * Split the simulation domain into regular rectangular tiles in 1D space.
 * No special time-skewing technique is used. This serves only as a reference
 * implementation.
 *
 * This implementation considers dependencies between electric and magnetic
 * field, the magnetic field update range is 1 unit small than electric
 * field's range, thus, it's safe to update electric and magnetic fields in
 * a single step. However, it doesn't consider the problem of dependencies
 * at a tile's edge, so it's unsafe for multi-threading due to lack of
 * synchronization.
 */
Tiles computeRectangularTiles1D(int totalWidth, int blkWidth, int blkHalfTimesteps)
{
	if (blkHalfTimesteps != 2)
	{
		/*
		 * One timestep contains two half timesteps, one electric, one
		 * magnetic. Multi-timestep techniques (time skewing) are not
		 * supported.
		 */
		std::exit(1);
	}

	std::vector<Block> blockList;

	/* at the beginning, divide the axis into several tiles */
	int numBlocks    = totalWidth / blkWidth;
	int numRemainder = totalWidth % blkWidth;

	/* For leftover cells, we allocate another block */
	if (numRemainder > 0) {
		numBlocks += 1;
	}

	blockList.resize(numBlocks);

	for (size_t i = 0; i < blockList.size(); i++) {
		Block block;

		for (int t = 0; t < 2; t++) {
			if (t % 2 == 0) {
				/* electric field range */
				Range startStop = Range(
					i * blkWidth,
					i * blkWidth + blkWidth - 1
				);
				block.push_back(startStop);
			}
			else {
				/* magnetic field range */
				Range startStop = Range(
					i * blkWidth - 1,
					i * blkWidth + blkWidth - 2
				);

				if (startStop.first < 0) {
					startStop.first = 0;
				}

				block.push_back(startStop);
			}
		}

		blockList[i] = block;
	}

	/* TILES_RECTANGULAR has one phase. */
	Tiles tiles;
	tiles.type = TILES_RECTANGULAR;
	tiles.phases = 1;
	tiles.array.resize(tiles.phases);
	tiles.array[0] = blockList;

	return tiles;
}

std::vector<std::vector<Tiles3D>> computeRectangularTiles3D(
	int totalWidth[3],
	int blkWidth[3],
	int numThreads
)
{
	Tiles tilesX = computeRectangularTilesNoDeps1D(totalWidth[0], blkWidth[0], 2);
	Tiles tilesY = computeRectangularTilesNoDeps1D(totalWidth[1], blkWidth[1], 2);
	Tiles tilesZ = computeRectangularTilesNoDeps1D(totalWidth[2], blkWidth[2], 2);

	std::vector<std::vector<Tiles3D>> tilesPerPhasePerThread;
	tilesPerPhasePerThread.resize(numThreads);
	for (auto& tilesPerPhase : tilesPerPhasePerThread) {
		/* only one phase */
		tilesPerPhase.resize(1);
	}

	int assignedthread = 0;
	int blkHalfTimesteps = 2;

	for (int phaseX = 0; phaseX < tilesX.phases; phaseX++) {
		for (int phaseY = 0; phaseY < tilesY.phases; phaseY++) {
			for (int phaseZ = 0; phaseZ < tilesZ.phases; phaseZ++) {
				for (int x = 0; x < tilesX.array[phaseX].size(); x++) {
					for (int y = 0; y < tilesY.array[phaseY].size(); y++) {
						for (int z = 0; z < tilesZ.array[phaseZ].size(); z++) {
							for (int t = 0; t < blkHalfTimesteps; t += 2) {
								Range3D r;

								r.voltageStart[0] = tilesX.array[phaseX][x][t].first;
								r.voltageStart[1] = tilesY.array[phaseY][y][t].first;
								r.voltageStart[2] = tilesZ.array[phaseZ][z][t].first;

								r.voltageStop[0]  = tilesX.array[phaseX][x][t].second;
								r.voltageStop[1]  = tilesY.array[phaseY][y][t].second;
								r.voltageStop[2]  = tilesZ.array[phaseZ][z][t].second;

								r.currentStart[0] = tilesX.array[phaseX][x][t + 1].first;
								r.currentStart[1] = tilesY.array[phaseY][y][t + 1].first;
								r.currentStart[2] = tilesZ.array[phaseZ][z][t + 1].first;

								r.currentStop[0]  = tilesX.array[phaseX][x][t + 1].second;
								r.currentStop[1]  = tilesY.array[phaseY][y][t + 1].second;
								r.currentStop[2]  = tilesZ.array[phaseZ][z][t + 1].second;

								bool omit = false;
								for (int n = 0; n < 3; n++) {
									if (r.voltageStart[n] == -1 && r.voltageStop[n] == -1) {
										omit = true;
									}
									if (r.currentStart[n] == -1 && r.currentStop[n] == -1) {
										omit = true;
									}
								}
								if (!omit) {
									tilesPerPhasePerThread[assignedthread][0].push_back(r);
									assignedthread = (assignedthread + 1) % numThreads;
								}
							}
						}
					}
				}
			}
		}
	}
	return tilesPerPhasePerThread;
}

/*
 * Calculate parallelogram tiles in 1D space + 1D time, according to:
 *
 *   Fukaya, T., & Iwashita, T. (2018).
 *   Time-space tiling with tile-level parallelism for the 3D FDTD method.
 *   Proceedings of the International Conference on High Performance
 *   Computing in Asia-Pacific Region - HPC Asia 2018.
 *   doi: 10.1145/3149457.3149478
 *
 * "blkWidth" is the width of a tile, and "blkTimesteps" is the height
 * of the tile, in other words, the number of timesteps to be calculated
 * simultaneously.
 *
 * Without loss of generality, consider the case of tiling on the X axis.
 * In FDTD, the magnetic field at (x, t) is calculated from the electric
 * field at (x + 1, t - 1) at the bottom right. Meanwhile the electric
 * field at (x, t) is calculated from the magnetic field at (x - 1, t - 1)
 * at the bottom left.
 *
 * ^
 * | t      blkWidth
 * |       |--------|
 * | AAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHH
 * | AAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHH
 * | AAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHH
 * | AAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHH
 * | AAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHH
 * | AAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGH
 * | AAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGH
 * | AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGG
 * |------------------------------------------------------------------------>
 *                                                                       x
 * (See Figure 8 (a) in the paper for a colored version)
 *
 * Thus, the 2D simulation space-time can be broken into parallelogram
 * tiles, each tile is the minimum unit of work and many timesteps
 * are calculated at once. There's still data dependency between tiles,
 * so each tile must be processed serially. 3D and 4D space-time can
 * also be tiled by applying X-T, Y-T, and Z-T recursively.
 *
 * Since each tile can fit in CPU cache, the required DRAM loads and
 * stores are substantially reduced, increasing FDTD's simulation speed.
 *
 * Basically, at the beginning, we divide the X axis into several groups
 * with blkWidth. Then, if the next timestep is even (odd in 0-based index),
 * we shift each group to the left by 1 unit (if it's less than 0, it's
 * truncated). If the next timestep is odd (even in 0-based index), we do
 * not move.
 *
 * Note: More accurately, the "timestep" here means "half time step" since
 * one complete time step contains one electric step and one magnetic step,
 * but for simplicity we refer half-steps as a single timestep.
 */
Tiles computeParallelogramTiles1D(
	int totalWidth,
	int blkWidth,
	int blkTimesteps
)
{
	std::vector<Block> blockList;

	/* at the beginning, divide the axis into several tiles */
	int numBlocks    = totalWidth / blkWidth;
	int numRemainder = totalWidth % blkWidth;

	/* For leftover cells, we allocate another block */
	if (numRemainder > 0) {
		numBlocks += 1;
	}

	/*
	 * The rightmost block is possibly still incomplete (what a
	 * headache) and truncated on the edge. This happens when the
	 * first timestep is a completely divided into blocks, but
	 * their widths decrease when we go forward in time, creating
	 * empty space. Thus we always allocate one more block and
	 * pretend the simulation space is large enough. We remove
	 * out-of-bound blocks as the last step.
	 */
	blockList.resize(numBlocks + 1);

	for (size_t i = 0; i < blockList.size(); i++) {
		Block block;
		block.reserve(blkTimesteps);

		Range startStop = Range(
			i * blkWidth,
			i * blkWidth + blkWidth - 1
		);

		block.push_back(startStop);
		blockList[i] = block;
	}

	for (int t = 1; t < blkTimesteps; t++) {
		for (size_t i = 0; i < blockList.size(); i++) {
			int prevStart = blockList[i][t - 1].first;
			int prevStop = blockList[i][t - 1].second;

			if (t % 2 == 0) {
				/* timestep is even, just copy the range */
				Range startStop = Range(
					prevStart, prevStop
				);
				blockList[i].push_back(startStop);
			}
			else {
				/* timestep is odd, shift 1 unit left */
				Range startStop = Range(
					prevStart - 1, prevStop - 1
				);

				/* truncate if off the left edge */
				if (prevStart - 1 < 0) {
					startStop.first = 0;
				}
				blockList[i].push_back(startStop);
			}
		}
	}

	/*
	 * The headache of the last incomplete tile continues. We need
	 * to remove the parts that are outside the boundary.
	 */
	for (int t = 0; t < blkTimesteps; t++) {
		for (size_t i = 0; i < blockList.size(); i++) {
			Range r;
			r = blockList[i][t];
			if (r.first > totalWidth - 1) {
				r.first = -1;
				r.second = -1;
			}
			if (r.second > totalWidth - 1) {
				r.second = totalWidth - 1;
			}
			blockList[i][t] = r;
		}
	}

	/* TILES_PARALLELOGRAM has one phase. */
	Tiles tiles;
	tiles.type = TILES_PARALLELOGRAM;
	tiles.phases = 1;
	tiles.array.resize(tiles.phases);
	tiles.array[0] = blockList;

	return tiles;
}

/*
 * Calculate diamond tiles in 1D space + 1D time, according to:
 *
 *   Fukaya, T., & Iwashita, T. (2018).
 *   Time-space tiling with tile-level parallelism for the 3D FDTD method.
 *   Proceedings of the International Conference on High Performance
 *   Computing in Asia-Pacific Region - HPC Asia 2018.
 *   doi: 10.1145/3149457.3149478
 *
 * "blkWidth" is the width of a tile, and "blkTimesteps" is the height
 * of the tile, in other words, the number of timesteps to be calculated
 * simultaneously.
 *
 * Without loss of generality, consider the case of tiling on the X axis.
 * In FDTD, the magnetic field at (x, t) is calculated from the electric
 * field at (x + 1, t - 1) at the bottom right. Meanwhile the electric
 * field at (x, t) is calculated from the magnetic field at (x - 1, t - 1)
 * at the bottom left.
 *
 * ^
 * | t      blkTimesteps + blkWidth - 1
 * |           |---------------|
 * | AAAAAAAAAAEEEEEEEEEEEEEEEEEBBBBBBBBBBFFFFFFFFFFFFFFFFFCCCCCCCCCCGGGGGG
 * | AAAAAAAAAAAEEEEEEEEEEEEEEEEBBBBBBBBBBBFFFFFFFFFFFFFFFFCCCCCCCCCCCGGGGG
 * | AAAAAAAAAAAEEEEEEEEEEEEEEEBBBBBBBBBBBBFFFFFFFFFFFFFFFCCCCCCCCCCCCGGGGG
 * | AAAAAAAAAAAAEEEEEEEEEEEEEEBBBBBBBBBBBBBFFFFFFFFFFFFFFCCCCCCCCCCCCCGGGG
 * | AAAAAAAAAAAAEEEEEEEEEEEEEBBBBBBBBBBBBBBFFFFFFFFFFFFFCCCCCCCCCCCCCCGGGG
 * | AAAAAAAAAAAAAEEEEEEEEEEEEBBBBBBBBBBBBBBBFFFFFFFFFFFFCCCCCCCCCCCCCCCGGG
 * | AAAAAAAAAAAAAEEEEEEEEEEEBBBBBBBBBBBBBBBBFFFFFFFFFFFCCCCCCCCCCCCCCCCGGG
 * | AAAAAAAAAAAAAAEEEEEEEEEEBBBBBBBBBBBBBBBBBFFFFFFFFFFCCCCCCCCCCCCCCCCCGG
 * |------------------------------------------------------------------------>
 *                                                                       x
 * (See Figure 8 (b) in the paper for a colored version)
 *
 * Thus, the 2D simulation space-time can be broken into diamond tiles.
 * Two types of diamond tiles exist: if a tile becomes shorter over time,
 * it's called a "mountain" tile, such as tile A or B in the diagram. If a
 * tile becomes longer over time, it's called a "valley" tile, such as
 * tile E or F. Both tiles always exist in a interleaved manner, i.e. a
 * valley between two mountains.
 *
 * Each tile is the minimum unit of work and many timesteps are calculated
 * at once. There's NO data dependency within all mountains or all valleys,
 * so both tiles can be processed in parallel in 2 passes, first for all
 * mountains, then for all valleys.
 *
 * Since each tile can fit in CPU cache, the required DRAM loads and
 * stores are substantially reduced, in addition, intra-tile parallelism
 * exists. Both increase FDTD's simulation speed.
 *
 * 3D and 4D space-time can also be tiled by applying X-T, Y-T, and Z-T
 * recursively. Parallelogram and diamond tiles can be mixed on different
 * axis. For example, a diamond-diamond-parallelogram tiling over the
 * XYZT space requires 4 passes, while a diamond-diamond-diamond tiling
 * requires 8 passes.
 *
 * Basically, at the beginning, we divide the X axis into several mountains
 * and valleys. The width of a tile is defined to be the shortest span of a
 * mountain (top) or valley (bottom). We then calculate the longest span of
 * a mountain (bottom) or valley (top), which is blkWidth + blkTimesteps.
 *
 * Then, for a valley, if the next timestep is even (odd if 0-based), we
 * expand the block to the left by 1 unit, if the next timestamp is odd,
 * we expand to the right, we expand the block to the right by 1 unit.
 * For a mountain, if the next timestep is even (odd if 0-based), we
 * shorten its width from right to the left by 1 unit, if the next time-
 * stamp is odd, we shorten its width from left to the right by 1 unit.
 * If a mountain or valley is moving outside the edge, it's truncated.
 *
 * Another problem is how to choose the beginning of a tile, since the
 * possible decompositions of mountains and valleys are not unique. To
 * solve this problem, we first calculate the top width of a mountain
 * and the bottom width of a valley. Next, we always fill the axis, at
 * the last timestep, with a mountain top and a valley top. Finally we
 * iterate timestamps in reverse, moving downwards.
 */
Tiles computeDiamondTiles1D(int totalWidth, int blkWidth, int blkTimesteps)
{
	/*
	 * The width of a block is defined to be the shortest span
	 * of a mountain (top) or valley (bottom).
	 */
	int blkSpanMin = blkWidth;

	/*
	 * The longest span of a block is the mountain top or valley
	 * bottom.
	 */
	int blkSpanMax = blkWidth + blkTimesteps - 1;

	/* Calculate the total number of blocks at the last timestep. */
	int numBlocks    = totalWidth / (blkSpanMin + blkSpanMax) * 2;
	int numRemainder = totalWidth % (blkSpanMin + blkSpanMax);

	/* For leftover blocks, we add more mountains and valleys */
	for (int i = 0; numRemainder > 0; i++) {
		if (i % 2 == 0) {
			/* mountain */
			numBlocks++;
			numRemainder -= blkSpanMin;
		}
		else {
			/* valley */
			numBlocks++;
			numRemainder -= blkSpanMax;
		}
	}

	std::vector<Block> blockList;

	/*
	 * The rightmost block is possibly still incomplete (what a
	 * headache) and truncated on the edge. This happens when the
	 * last block is a full mountain, but its width decreases when
	 * we go back in time, creating empty space. Thus we always
	 * allocate one block more and pretend there's one more full
	 * block. We'll pretend the simulation space is large enough,
	 * and we remove out-of-bound blocks as the last step.
	 */
	blockList.resize(numBlocks + 1);

	/*
	 * Let's now consider how the axis should be splitted at the
	 * last timestep. It should be in the order of a mountain top
	 * (blkSpanMin), then a valley bottom (blkSpanMax), etc.
	 */
	int lastStop = -1;
	for (size_t i = 0; i < blockList.size(); i++) {
		Block block;
		block.resize(blkTimesteps);

		Range startStop;

		if (i % 2 == 0) {
			/* mountain */
			startStop = Range(
				lastStop + 1,
				lastStop + blkSpanMin
			);
		}
		else {
			/* valley */
			startStop = Range(
				lastStop + 1,
				lastStop + blkSpanMax
			);
		}
		lastStop = startStop.second;

		block[blkTimesteps - 1] = startStop;
		blockList[i] = block;
	}

 	/* Finally we iterate timestamps in reverse, moving downwards. */
	for (int t = blkTimesteps - 2; t >= 0; t--) {
		for (size_t i = 0; i < blockList.size(); i++) {
			int prevStart = blockList[i][t + 1].first;
			int prevStop = blockList[i][t + 1].second;

			Range startStop;
			if (t % 2 != 0 && i % 2 == 0) {
				/*
				 * timestep is odd, block is a mountain,
				 * expand 1 unit to the left
				 */
				startStop = Range(
					prevStart - 1, prevStop
				);
			}
			else if (t % 2 == 0 && i % 2 == 0) {
				/*
				 * timestep is even, block is a mountain,
				 * expand 1 unit to the right
				 */
				startStop = Range(
					prevStart, prevStop + 1
				);
			}
			else if (t % 2 != 0 && i % 2 != 0) {
				/*
				 * timestep is odd, block is a valley,
				 * reduce 1 unit from right to the left
				 */
				startStop = Range(
					prevStart, prevStop - 1
				);
			}
			else if (t % 2 == 0 && i % 2 != 0) {
				/*
				 * timestep is even, block is a valley,
				 * reduce 1 unit from left to the right
				 */
				startStop = Range(
					prevStart + 1, prevStop
				);
			}

			/* truncate if off the left edge */
			if (startStop.first < 0) {
				startStop.first = 0;
			}
			blockList[i][t] = startStop;
		}
	}

	/*
	 * The headache of the last incomplete tile continues. We need
	 * to remove the parts that are outside the boundary.
	 */
	for (int t = 0; t < blkTimesteps; t++) {
		for (size_t i = 0; i < blockList.size(); i++) {
			Range r;
			r = blockList[i][t];
			if (r.first > totalWidth - 1) {
				r.first = -1;
				r.second = -1;
			}
			if (r.second > totalWidth - 1) {
				r.second = totalWidth - 1;
			}
			blockList[i][t] = r;
		}
	}

	/* TILES_DIAMOND has two phases. */
	Tiles tiles;
	tiles.type = TILES_DIAMOND;
	tiles.phases = 2;
	tiles.array.resize(tiles.phases);

	for (size_t i = 0; i < blockList.size(); i++) {
		if (i % 2 == 0) {
			/* all mountain blocks go to phase 1 */
			tiles.array[0].push_back(blockList[i]);
		}
		else {
			/* all valley blocks go to phase 2 */
			tiles.array[1].push_back(blockList[i]);
		}
	}

	return tiles;
}

/*
 * Combine tiles calculated seperately for the X, Y, Z axis to a
 * single vector of Tiles. This is essentially a dry-run of the
 * timestepping code, but it helps improving code clarity of the
 * main engine.
 */
Tiles3D combineTilesTo3D(Tiles tilesX, Tiles tilesY, Tiles tilesZ, int blkHalfTimesteps)
{
	Tiles3D tiles;

	for (int phaseX = 0; phaseX < tilesX.phases; phaseX++) {
		for (int phaseY = 0; phaseY < tilesY.phases; phaseY++) {
			for (int phaseZ = 0; phaseZ < tilesZ.phases; phaseZ++) {
				for (int x = 0; x < tilesX.array[phaseX].size(); x++) {
					for (int y = 0; y < tilesY.array[phaseY].size(); y++) {
						for (int z = 0; z < tilesZ.array[phaseZ].size(); z++) {
							for (int t = 0; t < blkHalfTimesteps; t += 2) {
								Range3D r;

								r.timestep = t / 2;

								r.voltageStart[0] = tilesX.array[phaseX][x][t].first;
								r.voltageStart[1] = tilesY.array[phaseY][y][t].first;
								r.voltageStart[2] = tilesZ.array[phaseZ][z][t].first;

								r.voltageStop[0]  = tilesX.array[phaseX][x][t].second;
								r.voltageStop[1]  = tilesY.array[phaseY][y][t].second;
								r.voltageStop[2]  = tilesZ.array[phaseZ][z][t].second;

								r.currentStart[0] = tilesX.array[phaseX][x][t + 1].first;
								r.currentStart[1] = tilesY.array[phaseY][y][t + 1].first;
								r.currentStart[2] = tilesZ.array[phaseZ][z][t + 1].first;

								r.currentStop[0]  = tilesX.array[phaseX][x][t + 1].second;
								r.currentStop[1]  = tilesY.array[phaseY][y][t + 1].second;
								r.currentStop[2]  = tilesZ.array[phaseZ][z][t + 1].second;

								bool omit = false;
								for (int n = 0; n < 3; n++) {
									if (r.voltageStart[n] == -1 && r.voltageStop[n] == -1) {
										omit = true;
									}
									if (r.currentStart[n] == -1 && r.currentStop[n] == -1) {
										omit = true;
									}
								}
								if (!omit) {
									tiles.push_back(r);
								}
							}
						}
					}
				}
			}
		}
	}
	return tiles;
}

std::vector<std::vector<Tiles3D>> combineTilesTo3D(
	Tiles tilesX, Tiles tilesY, Tiles tilesZ,
	int blkHalfTimesteps,
	int numThreads
)
{
	/*
	 * First, we need to find one dimension that is parallelizable
	 * (i.e. uses diamond tiling).
	 */
	int parallelAxis = -1;
	if (tilesZ.type == TILES_DIAMOND) {
		parallelAxis = 'Z';
	}
	else if (tilesY.type == TILES_DIAMOND) {
		parallelAxis = 'Y';
	}
	else if (tilesX.type == TILES_DIAMOND) {
		parallelAxis = 'X';
	}

	if (parallelAxis == -1 && numThreads != 1) {
		fprintf(stderr, "no parallelization possible.\n");
		std::exit(1);
	}

	int totalPhases = tilesX.phases * tilesY.phases * tilesZ.phases;
	std::vector<std::vector<Tiles3D>> tilesPerPhasePerThread;
	tilesPerPhasePerThread.resize(numThreads);

	for (auto& tilesPerPhase : tilesPerPhasePerThread) {
		tilesPerPhase.resize(totalPhases);
	}

	int phaseXYZ[3] = {0, 0, 0};  /* X, Y, Z */
	int assignedThread = 0;
	for (int phase = 0; phase < totalPhases; phase++) {
		fprintf(stderr, "phase(%d, %d, %d)\n", phaseXYZ[0], phaseXYZ[1], phaseXYZ[2]);
		int phaseX = phaseXYZ[0];
		int phaseY = phaseXYZ[1];
		int phaseZ = phaseXYZ[2];

		for (int x = 0; x < tilesX.array[phaseX].size(); x++) {
			for (int y = 0; y < tilesY.array[phaseY].size(); y++) {
				for (int z = 0; z < tilesZ.array[phaseZ].size(); z++) {
					for (int t = 0; t < blkHalfTimesteps; t += 2) {
						Range3D r;

						r.timestep = t / 2;

						r.voltageStart[0] = tilesX.array[phaseX][x][t].first;
						r.voltageStart[1] = tilesY.array[phaseY][y][t].first;
						r.voltageStart[2] = tilesZ.array[phaseZ][z][t].first;

						r.voltageStop[0]  = tilesX.array[phaseX][x][t].second;
						r.voltageStop[1]  = tilesY.array[phaseY][y][t].second;
						r.voltageStop[2]  = tilesZ.array[phaseZ][z][t].second;

						r.currentStart[0] = tilesX.array[phaseX][x][t + 1].first;
						r.currentStart[1] = tilesY.array[phaseY][y][t + 1].first;
						r.currentStart[2] = tilesZ.array[phaseZ][z][t + 1].first;

						r.currentStop[0]  = tilesX.array[phaseX][x][t + 1].second;
						r.currentStop[1]  = tilesY.array[phaseY][y][t + 1].second;
						r.currentStop[2]  = tilesZ.array[phaseZ][z][t + 1].second;

						bool omit = false;
						for (int n = 0; n < 3; n++) {
							if (r.voltageStart[n] == -1 && r.voltageStop[n] == -1) {
								omit = true;
							}
							if (r.currentStart[n] == -1 && r.currentStop[n] == -1) {
								omit = true;
							}
						}
						if (!omit) {
							tilesPerPhasePerThread[assignedThread][phase].push_back(r);
						}
					}
					if (parallelAxis == 'Z') {
						assignedThread = (assignedThread + 1) % numThreads;
					}
				}
				if (parallelAxis == 'Y') {
					assignedThread = (assignedThread + 1) % numThreads;
				}
			}
			if (parallelAxis == 'X') {
				assignedThread = (assignedThread + 1) % numThreads;
			}
		}

		/*
		 * Carry propagation for the (phaseX, phaseY, phaseZ) counter.
		 * This is equivelant to three nested for loops.
		 */
		phaseXYZ[2]++;

		if (phaseXYZ[2] > tilesZ.phases - 1) {
			phaseXYZ[2] = 0;
			phaseXYZ[1]++;
		}
		if (phaseXYZ[1] > tilesY.phases - 1) {
			phaseXYZ[1] = 0;
			phaseXYZ[0]++;
		}
		if (phaseXYZ[0] > tilesX.phases - 1) {
			/*
			 * No need to do anything if phaseX overflows, since the outer
			 * for loop will terminate when it reaches totalPhases.
			 */
		}
	}

	/*
	 * Tiling may be applied to only selected dimension, for example,
	 * X and Y but not Z. In this case, there will be phases that is
	 * empty across all thread. Delete them to avoid synchronization
	 * overhead.
	 */
	std::vector<int> emptyPhaseList;
	for (int phase = 0; phase < totalPhases; phase++) {
		int removePhase = true;

		for (int thread = 0; thread < numThreads; thread++) {
			if (tilesPerPhasePerThread[thread][phase].size() > 0) {
				removePhase = false;
			}
		}

		if (removePhase) {
			emptyPhaseList.push_back(phase);
		}
	}

	for (auto& tilesPerPhase : tilesPerPhasePerThread) {
		int phase = 0;
		int maxPhase = tilesPerPhase.size();
		int idxPhase = 0;
		while (phase < maxPhase) {
			if (std::find(emptyPhaseList.begin(), emptyPhaseList.end(), phase) != emptyPhaseList.end()) {
				tilesPerPhase.erase(tilesPerPhase.begin() + idxPhase);
			}
			else {
				idxPhase++;
			}
			phase++;
		}
	}

	return tilesPerPhasePerThread;
}

void visualizeTiles1D(Tiles tiles, int totalWidth, int blkTimesteps)
{
	int simSpace[totalWidth * blkTimesteps];
	memset(simSpace, '!', totalWidth * blkTimesteps * sizeof(int));

	char blkID = 'A';

	for (int phase = 0; phase < tiles.phases; phase++) {
		auto tileList = tiles.array[phase];

		for (size_t i = 0; i < tileList.size(); i++) {
			Block tile = tileList[i];
			for (size_t t = 0; t < tile.size(); t++) {
				Range r = tile[t];
				if (r.first == -1 || r.second == -1) {
					continue;
				}
				for (int loc = r.first; loc <= r.second; loc++) {
					if (t * totalWidth + loc > totalWidth * blkTimesteps - 1) {
						fprintf(stderr, "tile %ld timestep %ld is broken\n", i, t);
						fprintf(stderr, "from %d to %d\n", r.first, r.second);
						fprintf(stderr, "@loc %d\n", loc);
						continue;
					}
					simSpace[t * totalWidth + loc] = blkID;
				}
			}

			blkID++;
		}
	}

	for (int i = blkTimesteps - 1; i >= 0; i--) {
		for (int j = 0; j < totalWidth; j++) {
			putchar(simSpace[i * totalWidth + j]);;
		}
		putchar('\n');
	}
}

void traceTiles3D(Tiles3D tileList)
{
	for (auto& tile : tileList) {
		fprintf(stderr, "UpdateVoltages (%d, %d) (%d, %d) (%d, %d)\n",
			tile.voltageStart[0], tile.voltageStop[0],
			tile.voltageStart[1], tile.voltageStop[1],
			tile.voltageStart[2], tile.voltageStop[2]
		);

		fprintf(stderr, "UpdateCurrents (%d, %d) (%d, %d) (%d, %d)\n",
			tile.currentStart[0], tile.currentStop[0],
			tile.currentStart[1], tile.currentStop[1],
			tile.currentStart[2], tile.currentStop[2]
		);
	}
}

#ifdef TEST
void traceRectangularTilesExecution(void)
{
	Tiles tilesX = computeRectangularTiles1D(100, 10, 2);
	Tiles tilesY = computeRectangularTiles1D(100, 10, 2);
	Tiles tilesZ = computeRectangularTiles1D(100, 10, 2);
	Tiles3D tiles = combineTilesTo3D(tilesX, tilesY, tilesZ, 2);
	traceTiles3D(tiles);
}

void traceMultithreadedRectangularTilesExecution(void)
{
	int totalSizes[3] = {147, 335, 77};
	int blkSizes[3] = {147, 335, 77};
	int numThreads = 1;
	auto tilesPerPhasePerThread = computeRectangularTiles3D(totalSizes, blkSizes, numThreads);

	int longestTileCountPerThread = 0;
	for (auto& tilesPerPhase : tilesPerPhasePerThread) {
		if (tilesPerPhase[0].size() > longestTileCountPerThread) {
			longestTileCountPerThread = tilesPerPhase[0].size();
		}
	}

	for (int tile = 0; tile < longestTileCountPerThread; tile++) {
		for (int thread = 0; thread < numThreads; thread++) {
			auto tilesWithinThread = tilesPerPhasePerThread[thread][0];
			if (tile > tilesWithinThread.size() - 1) {
				continue;
			}

			auto tileObj = tilesWithinThread[tile];
			fprintf(stderr, "UpdateVoltages (%02d, %02d) (%02d, %02d) (%02d, %02d)",
				tileObj.voltageStart[0], tileObj.voltageStop[0],
				tileObj.voltageStart[1], tileObj.voltageStop[1],
				tileObj.voltageStart[2], tileObj.voltageStop[2]
			);
			fprintf(stderr, "    ");
		}
		fprintf(stderr, "\n");

		for (int thread = 0; thread < numThreads; thread++) {
			auto tilesWithinThread = tilesPerPhasePerThread[thread][0];
			if (tile > tilesWithinThread.size() - 1) {
				continue;
			}

			auto tileObj = tilesWithinThread[tile];
			fprintf(stderr, "UpdateCurrents (%02d, %02d) (%02d, %02d) (%02d, %02d)",
				tileObj.currentStart[0], tileObj.currentStop[0],
				tileObj.currentStart[1], tileObj.currentStop[1],
				tileObj.currentStart[2], tileObj.currentStop[2]
			);
			fprintf(stderr, "    ");
		}
		fprintf(stderr, "\n");
	}
}

void visualizeParallelogramTiles1D(void)
{
	Tiles tilesX = computeParallelogramTiles1D(70, 10, 8);
	visualizeTiles1D(tilesX, 70, 8);
}

void traceParallelogramTilesExecution(void)
{
	Tiles tilesX = computeParallelogramTiles1D(100, 10, 2);
	Tiles tilesY = computeParallelogramTiles1D(100, 10, 2);
	Tiles tilesZ = computeParallelogramTiles1D(100, 10, 2);
	Tiles3D tiles = combineTilesTo3D(tilesX, tilesY, tilesZ, 2);
	traceTiles3D(tiles);
}

void visualizeDiamondTiles1D(void)
{
	Tiles tilesX = computeDiamondTiles1D(70, 10, 8);
	visualizeTiles1D(tilesX, 70, 8);
}

void traceDiamondTilesExecution(void)
{
	Tiles tilesX = computeDiamondTiles1D(100, 10, 2);
	Tiles tilesY = computeDiamondTiles1D(100, 10, 2);
	Tiles tilesZ = computeDiamondTiles1D(100, 10, 2);
	Tiles3D tiles = combineTilesTo3D(tilesX, tilesY, tilesZ, 2);
	traceTiles3D(tiles);
}

void showWorkPerThreads(std::vector<std::vector<Tiles3D>>& tilesPerStagePerThread)
{
	int thread = 0;

	for (auto& tilesPerStage : tilesPerStagePerThread) {
		fprintf(stderr, "thread %d: ", thread);
		for (auto& tiles : tilesPerStage) {
			fprintf(stderr, "%ld, ", tiles.size());
		}
		fprintf(stderr, "\n");
		thread++;
	}
}

int main(void)
{
	// Tiles tilesX = computeDiamondTiles1D(100, 10, 2);
	// Tiles tilesY = computeDiamondTiles1D(100, 10, 2);
	// Tiles tilesZ = computeParallelogramTiles1D(100, 10, 2);
	// auto retval = combineTilesTo3D(tilesX, tilesY, tilesZ, 2, 32);

	// traceMultithreadedRectangularTilesExecution();

	// visualizeParallelogramTiles1D();
	// visualizeDiamondTiles1D();

	// traceRectangularTilesExecution()
	// traceParallelogramTilesExecution();
	// traceDiamondTilesExecution();

	return 0;
}
#endif
