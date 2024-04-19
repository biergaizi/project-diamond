// BSD Zero Clause License
// 
// Copyright (C) 2024 Yifeng Li
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <cassert>
#include <iostream>

#define TILING_NO_HACK
#include "tiling.hpp"
using namespace Tiling;

Plan1D
Tiling::computeParallelogramTiles(
	size_t totalWidth, size_t tileWidth,
	size_t halfTimesteps
)
{
	const size_t tileMinWidth = tileWidth - halfTimesteps / 2;
	const size_t tileMaxWidth = tileWidth;

	if (halfTimesteps % 2 != 0) {
		throw std::invalid_argument(
			"halfTimesteps must be even."
		);
	}
	if (halfTimesteps / 2 >= tileMaxWidth) {
		throw std::invalid_argument(
			"Timestep size is too large for tile size."
		);
	}

	TileList1D tileList;
	Range1D<size_t> range = {
		0,
		std::min(tileMaxWidth - 1, totalWidth - 1)
	};

	// Split totalWidth into tiles, each is tileMinWidth long.
	while (range.first <= totalWidth - 1) {
		// create tile
		Tile1D tile;
		tile.reserve(halfTimesteps);

		// create 1st half timestep within tile
		tile.push_back(range);
		tileList.push_back(tile);

		range.first = range.last + 1;
		range.last = std::min(range.last + tileMinWidth, totalWidth - 1);
	}

	// Iterate all tiles and complete the remaining half timesteps ranges
	// within each tile.
	for (size_t tileId = 0; tileId < tileList.size(); tileId++) {
		Tile1D& tile = tileList[tileId];

		for (size_t halfTs = 1; halfTs < halfTimesteps; halfTs++) {
			Range1D<size_t> prevRange = tile[halfTs - 1];

			Range1D<ssize_t> shift;
			if (halfTs % 2 == 0) {
				// timestep is even, don't shift
				shift = Range1D<ssize_t>{0, 0};
			}
			else {
				// timestep is odd, shift 1 unit to left
				shift = Range1D<ssize_t>{-1, -1};
			}

			// truncate range if beyond boundaries
			if (tileId == 0) {
				shift.first = 0;
			}
			if (prevRange.first == 0 && shift.first < 0) {
				throw std::invalid_argument("halfTs too large.");
			}
			if (tileId == tileList.size() - 1 ||
				prevRange.last + shift.last > totalWidth - 1
			) {
				shift.last = 0;
			}

			Range1D<size_t> currRange = {
				prevRange.first + shift.first,
				prevRange.last + shift.last
			};

			tile.push_back(currRange);
		}
	}

	// In FDTD, the last magnetic cells at the right boundary depends
	// on cells outside the simulation grid, so they can't be calculated.
	// Remove these cells.
	for (size_t tileId = 0; tileId < tileList.size(); tileId++) {
		Tile1D& tile = tileList[tileId];

		for (size_t halfTs = 1; halfTs < halfTimesteps; halfTs++) {
			Range1D<size_t>& range = tile[halfTs];

			if (halfTs % 2 == 1 && range.last > totalWidth - 2) {
				range.last = totalWidth - 2;
			}
		}
	}

	// parallelogram tiling only has 1 stage
	Plan1D plan(1);
	plan[0] = tileList;
	return plan;
}

Plan1D
Tiling::computeTrapezoidTiles(
	size_t totalWidth, size_t tileWidth,
	size_t halfTimesteps
)
{
	const size_t tileMinWidth = tileWidth - halfTimesteps + 1;
	const size_t tileMaxWidth = tileWidth;
	const size_t mountainOverlapWidth = halfTimesteps / 2 - 1;
	const size_t valleyOverlapWidth = halfTimesteps / 2;

	if (halfTimesteps % 2 != 0) {
		throw std::invalid_argument(
			"halfTimesteps must be even."
		);
	}
	if (halfTimesteps + 1 >= tileMaxWidth) {
		throw std::invalid_argument(
			"Timestep size is too large for tile size."
		);
	}

	TileList1D tileList;
	Range1D<size_t> range = {
		0,
		std::min(tileMaxWidth - 1, totalWidth)
	};

	while (range.first <= totalWidth - 1) {
		// create tile
		Tile1D tile;
		tile.reserve(halfTimesteps);

		// create 1st half timestep within tile
		tile.push_back(range);
		tileList.push_back(tile);

		range.first = range.last + 1;
		if (tileList.size() % 2 == 0 || totalWidth == tileWidth) {
			range.last = std::min(
				range.first + tileMaxWidth - 1,
				totalWidth - 1
			);
		}
		else {
			range.last = std::min(
				range.first + tileMinWidth - 1,
				totalWidth - 1
			);

			// Another special edge case: if tile (n - 1) is a valley
			// and it eventually expands to the rightmost cell, the
			// last mountain tile n would have truncated timesteps.
			// Thus, merge the both tiles into a single tile.
			if (range.last + mountainOverlapWidth >= totalWidth - 1) {
				range.last = totalWidth - 1;
			}
		}
	}

	// Iterate all tiles and complete the remaining half timesteps ranges
	// within each tile.
	for (size_t tileId = 0; tileId < tileList.size(); tileId++) {
		Tile1D& tile = tileList[tileId];

		for (size_t halfTs = 1; halfTs < halfTimesteps; halfTs++) {
			Range1D<size_t> prevRange = tile[halfTs - 1];
			Range1D<ssize_t> shift;

			if (tileId % 2 == 0) {
				// mountain
				if (halfTs % 2 == 1) {
					// timestep is odd, shrink right edge by 1 unit
					shift = Range1D<ssize_t>{0, -1};
				}
				else {
					// timestep is even, shrink left edge by 1 unit
					shift = Range1D<ssize_t>{1, 0};
				}
			}
			else {
				// valley
				if (halfTs % 2 == 1) {
					// timestep is odd, grow left edge by 1 unit
					shift = Range1D<ssize_t>{-1, 0};
				}
				else {
					// timestep is even, grow right edge by 1 unit
					shift = Range1D<ssize_t>{0, 1};
				}
			}

			if (tileId == 0) {
				shift.first = 0;
			}
			if (tileId == tileList.size() - 1
				|| prevRange.last + shift.last > totalWidth - 1
			) {
				shift.last = 0;
			}

			Range1D<size_t> currRange = {
				prevRange.first + shift.first,
				prevRange.last + shift.last
			};

			tile.push_back(currRange);
		}
	}

	// In FDTD, the last magnetic cells at the right boundary depends
	// on cells outside the simulation grid, so they can't be calculated.
	// Remove these cells.
	for (size_t tileId = 0; tileId < tileList.size(); tileId++) {
		Tile1D& tile = tileList[tileId];

		for (size_t halfTs = 1; halfTs < halfTimesteps; halfTs++) {
			Range1D<size_t>& range = tile[halfTs];

			if (halfTs % 2 == 1 && range.last > totalWidth - 2) {
				range.last = totalWidth - 2;
			}
		}
	}

	// trapezoid tiling has 2 stages
	Plan1D plan(2);

	for (size_t tileId = 0; tileId < tileList.size(); tileId++) {
		if (tileId % 2 == 0) {
			/* all mountain blocks go to stage 1 */
			plan[0].push_back(tileList[tileId]);
		}
		else {
			/* all valley blocks go to stage 2 */
			plan[1].push_back(tileList[tileId]);
		}
	}

	return plan;
}

Plan3D
Tiling::combineTilesTTT(const Plan1D& i, const Plan1D& j, const Plan1D& k)
{
	if (i.size() != 2 || j.size() != 2 || k.size() != 2) {
		throw std::invalid_argument("i/j/k must be trapezoid tiles.");
	}

	Plan3D plan(8);

	for (size_t stage = 0; stage < 8; stage++) {
		// 3-to-8 decoder:
		// Each dimension has two stages, mountain and valley.
		// Select a 3-tuple from all 8 possible combinations. 
		const TileList1D& tileListI = i[(stage >> 2) & 0x01];  // [0, 1]
		const TileList1D& tileListJ = j[(stage >> 1) & 0x01];  // [0, 1]
		const TileList1D& tileListK = k[(stage >> 0) & 0x01];  // [0, 1]
		TileList3D& tileListIJK = plan[stage];

		for (const Tile1D& tileI : tileListI) {
			for (const Tile1D& tileJ : tileListJ) {
				for (const Tile1D& tileK : tileListK) {
					if (tileI.size() != tileJ.size() ||
						tileJ.size() != tileK.size()
					) {
						throw std::invalid_argument(
							"all tiles must be time-aligned."
						);
					}
						
					Subtile3D subtile;
					for (size_t halfTs = 0; halfTs < tileI.size(); halfTs++) {
						subtile.push_back(Range3D<size_t>{
							{
								tileI[halfTs].first,
								tileJ[halfTs].first,
								tileK[halfTs].first
							},
							{
								tileI[halfTs].last,
								tileJ[halfTs].last,
								tileK[halfTs].last
							}
						});
					}

					Tile3D tile;
					tile.push_back(subtile);
					tileListIJK.push_back(tile);
				}
			}
		}
	}

	return plan;
}

Plan3D
Tiling::combineTilesTTP(const Plan1D& i, const Plan1D& j, const Plan1D& k)
{
	if (i.size() != 2 || j.size() != 2 || k.size() != 1) {
		throw std::invalid_argument(
			"i/j must be trapezoid tiles, k must be parallelogram tiles."
		);
	}

	Plan3D plan(4);

	for (size_t stage = 0; stage < 4; stage++) {
		// 2-to-4 decoder:
		// Each dimension has two stages, mountain and valley.
		// Select a 2-tuple from all 4 possible combinations. 
		const TileList1D& tileListI = i[(stage >> 1) & 0x01];  // [0, 1]
		const TileList1D& tileListJ = j[(stage >> 0) & 0x01];  // [0, 1]

		// The last dimension uses parallelogram instead of trapezoid
		// tiling, so it only has 1 stage and must be executed in serial
		// rather parallel.
		const TileList1D& tileListK = k[0];
		TileList3D& tileListIJK = plan[stage];

		// Instead of combining every tiles from I, J, K dimensions with
		// each other, here, only I and J dimensions are combined, then
		// each 2D tile is combined with all 1D tiles from dimension K
		// as a whole to create a single 3D tile.
		for (const Tile1D& tileI : tileListI) {
			for (const Tile1D& tileJ : tileListJ) {
				Tile3D tile;

				for (const Tile1D& tileK : tileListK) {
					Subtile3D subtile;

					for (size_t halfTs = 0; halfTs < tileI.size(); halfTs++) {
						if (tileI.size() != tileJ.size() ||
							tileJ.size() != tileK.size()
						) {
							throw std::invalid_argument(
								"all tiles must be time-aligned."
							);
						}

						subtile.push_back(Range3D<size_t>{
							{
								tileI[halfTs].first,
								tileJ[halfTs].first,
								tileK[halfTs].first
							},
							{
								tileI[halfTs].last,
								tileJ[halfTs].last,
								tileK[halfTs].last
							}
						});
					}
					tile.push_back(subtile);
				}
				tileListIJK.push_back(tile);
			}
		}
	}

	return plan;
}

Plan3D
Tiling::toLocalCoords(Plan3D plan)
{
	for (TileList3D& tileList : plan) {
		for (Tile3D& tile : tileList) {
			for (Subtile3D& subtile : tile) {
				for (Range3D<size_t>& range : subtile) {
					for (size_t n = 0; n < 3; n++) {
						range.first[n] = 0;
						range.last[n] = range.last[n] - subtile.first[n];
					}
				}
			}
		}
	}

	return plan;
}

void
Tiling::visualizeTiles(
	const Plan1D& plan,
	size_t totalWidth, size_t tileWidth,
	size_t halfTimesteps
)
{
	char simSpace[halfTimesteps][totalWidth];

	for (size_t halfTs = 0; halfTs < halfTimesteps; halfTs++) {
		for (size_t pos = 0; pos < totalWidth; pos++) {
			simSpace[halfTs][pos] = '!';
		}
	}

	assert(plan.size() <= 2);
	for (size_t stage = 0; stage < plan.size(); stage++) {
		char tileID;
		if (stage == 0) {
			tileID = '0';
		}
		else if (stage == 1) {
			tileID = 'A';
		}

		const TileList1D& tileList = plan[stage];

		for (const Tile1D& tile : tileList) {
			for (size_t halfTs = 0; halfTs < tile.size(); halfTs++) {
				const Range1D<size_t>& range = tile[halfTs];

				for (size_t pos = range.first; pos <= range.last; pos++) {
					if (pos > totalWidth - 1) {
						std::cerr << "tile " << pos << " timesteps " << halfTs
								  << "is broken from "
								  << range.first << " to " << range.last
								  << " @pos " << pos << "\n";
						continue;
					}
					simSpace[halfTs][pos] = tileID;
				}
			}

			tileID++;
		}
	}

	for (ssize_t halfTs = halfTimesteps - 1; halfTs >= 0; halfTs--) {
		for (size_t pos = 0; pos < totalWidth; pos++) {
			std::cout << simSpace[halfTs][pos];
		}
		std::cout << '\n';
	}
}
