/*
*	Copyright (C) 2010 Thorsten Liebig (Thorsten.Liebig@gmx.de)
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <ginac/ginac.h>
#include "array-nxyz.h"

/*
 * UpdateVoltages
 *
 * Calculate new electric field array "volt" based on
 * magnetic field "curr" and two electromagnetic field
 * operators "vv" and "vi", precalculated before starting
 * up simulation.
 *
 * Multiple threads may use startX and numX to partition
 * the 3D space across the X axis.
 */
void UpdateVoltages(
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::symbol>& vv,
	N_3DArray<GiNaC::symbol>& vi,
	unsigned int startX, unsigned int endX,
	unsigned int startY, unsigned int endY,
	unsigned int startZ, unsigned int endZ
)
{
	for (unsigned int x = startX; x <= endX; x++)
	{
		/*
		 * If we are at the beginning "0" of the axis, don't
		 * shift, otherwise, shift X/Y/Z by 1 to get the field
		 * from the adjacent cell.
		 */
		unsigned int prev_x = x > 0 ? 1 : 0;

		for (unsigned int y = startY; y <= endY; y++)
		{
			unsigned int prev_y = y > 0 ? 1 : 0;

			for (unsigned int z = startZ; z <= endZ; z++)
			{
				unsigned int prev_z = z > 0 ? 1 : 0;

				// do the updates here

				// note: each (x, y, z) cell has three polarizations
				// x, y, z, these are different from the cell's
				// coordinates (x, y, z)

				//for x polarization
				//fprintf(stderr, "write volt(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(2, %d, %d, %d)\n", x, y-prev_y, z);
				//fprintf(stderr, "    read curr(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(1, %d, %d, %d)\n", x, y, z-prev_z);

				volt(0, x, y, z) *= vv(0, x, y, z);
				volt(0, x, y, z) +=
				    vi(0, x, y, z) * (
				        curr(2, x, y       , z       ) -
				        curr(2, x, y-prev_y, z       ) -
				        curr(1, x, y       , z       ) +
				        curr(1, x, y       , z-prev_z)
				    );

				//for y polarization
				//fprintf(stderr, "write volt(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(0, %d, %d, %d)\n", x, y, z-prev_z);
				//fprintf(stderr, "    read curr(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(2, %d, %d, %d)\n", x-prev_x, y, z);

				volt(1, x, y, z) *= vv(1, x, y, z);
				volt(1, x, y, z) +=
				    vi(1, x, y, z) * (
				        curr(0, x       , y, z       ) -
				        curr(0, x       , y, z-prev_z) -
				        curr(2, x       , y, z       ) +
				        curr(2, x-prev_x, y, z       )
				    );

				//for z polarization
				//fprintf(stderr, "write volt(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(1, %d, %d, %d)\n", x-prev_x, y, z);
				//fprintf(stderr, "    read curr(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(0, %d, %d, %d)\n", x, y-prev_y, z);

				volt(2, x, y, z) *= vv(2, x, y, z);
				volt(2, x, y, z) +=
				    vi(2, x, y, z) * (
				        curr(1, x       , y       , z) -
				        curr(1, x-prev_x, y       , z) -
				        curr(0, x       , y       , z) +
				        curr(0, x       , y-prev_y, z)
				    );
			}
		}
	}
}

/*
 * UpdateCurrents
 *
 * Calculate new magnetic field array "curr" based on
 * electric field "volt" and two electromagnetic field
 * operators "ii" and "iv", precalculated before starting
 * simulation.
 *
 * Multiple threads may use startX and numX to partition
 * the 3D space across the X axis.
 *
 * Note that unlike the electric field, for magnetic field,
 * we need to stop the loop at y, z minus 1 due to the
 * interleaved nature of Yee's cell. It's also the caller's
 * responsibility to do this for numX.
 */
void UpdateCurrents(
	N_3DArray<GiNaC::ex>& curr,
	N_3DArray<GiNaC::ex>& volt,
	N_3DArray<GiNaC::symbol>& iv,
	N_3DArray<GiNaC::symbol>& ii,
	unsigned int startX, unsigned int endX,
	unsigned int startY, unsigned int endY,
	unsigned int startZ, unsigned int endZ
)
{
	for (unsigned int x = startX; x <= endX; x++)
	{
		for (unsigned int y = startY; y <= endY; y++)
		{
			for (unsigned int z = startZ; z <= endZ; z++)
			{
				//do the updates here

				// note: each (x, y, z) cell has three polarizations
				// x, y, z, these are different from the cell's
				// coordinates (x, y, z)

				//for x polarization
				//fprintf(stderr, "write curr(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(2, %d, %d, %d)\n", x, y+1, z);
				//fprintf(stderr, "    read volt(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(1, %d, %d, %d)\n", x, y, z+1);

				curr(0, x, y, z) *= ii(0, x, y, z);
				curr(0, x, y, z) +=
				    iv(0, x, y, z) * (
				        volt(2, x, y  , z  ) -
				        volt(2, x, y+1, z  ) -
				        volt(1, x, y  , z  ) +
				        volt(1, x, y  , z+1)
				    );

				//for y polarization
				//fprintf(stderr, "write curr(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(0, %d, %d, %d)\n", x, y, z+1);
				//fprintf(stderr, "    read volt(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(2, %d, %d, %d)\n", x+1, y, z);

				curr(1, x, y, z) *= ii(1, x, y, z);
				curr(1, x, y, z) +=
				    iv(1, x, y, z) * (
				        volt(0, x  , y, z  ) -
				        volt(0, x  , y, z+1) -
				        volt(2, x  , y, z  ) +
				        volt(2, x+1, y, z  )
				    );

				//for z polarization
				//fprintf(stderr, "write curr(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read curr(2, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(1, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(1, %d, %d, %d)\n", x+1, y, z);
				//fprintf(stderr, "    read volt(0, %d, %d, %d)\n", x, y, z);
				//fprintf(stderr, "    read volt(0, %d, %d, %d)\n", x, y+1, z);

				curr(2, x, y, z) *= ii(2, x, y, z);
				curr(2, x, y, z) +=
				    iv(2, x, y, z) * (
				        volt(1, x  , y  , z) -
				        volt(1, x+1, y  , z) -
				        volt(0, x  , y  , z) +
				        volt(0, x  , y+1, z)
				    );
			}
		}
	}
}
