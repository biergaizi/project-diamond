Project Diamond
====================

This repository contains experimental code for 3D FDTD domain decomposition
using parallelogram and diamond tiling.

The author is still in the process of trying to understand the tiling
techniques, so the code in its current form is broken, incorrect, and
ugly, so don't use it. After development is finished, I plan to write
a blog post to explain how everything works, which should happen before
the end of 2023, be patient. But if you still decide to read it, you're
on your own.

FDTD
---------

In microwave and high-speed digital electronics engineering, one often
needs to directly simulate electromagnetic waves by numerically solving
Maxwell's equations using the Finite-Difference Time-Domain (FDTD)
algorithm.

Unfortunately this algorithm loads a large number of cells into CPU
registers, performs only a few multiplications and adds, before writing
them back to memory. And you need to repeat this for every cell in 3D
space (which contains over 1 million cells) per timestep. A full simulation
contains at least many thousands of timesteps.

Thus, the FDTD kernel has an extremely low arithmetic intensity (around
0.25 FLOPS/bytes) and is bottlenecked by memory bandwidth, simulation
speed is extremely slow.

Loop Tiling and Time Skewing
------------------------------

But dispair not! There is a solution that can increase FDTD simulation
speed by ~100%, known as time skewing. Basically, instead of calculating
one step at a time in the entire 3D space, you divide the space into
multiple regions called tiles. Within each tiles, multiple timesteps are
calculated at once, as a result, the DRAM loads and stores are drastically
reduced, allowing for faster simulation speed.

The problem, however, is finding the correct shape of tiles with the
correct data dependencies. This is a classic problem in stencil computing,
and had been solved by supercomputing researchers. The solutions are
parallelogram tiling and diamond tilling, implemented according to:

* Fukaya, T., & Iwashita, T. (2018). Time-space tiling with tile-level parallelism for the 3D FDTD method. Proceedings of the International Conference on High Performance Computing in Asia-Pacific Region - HPC Asia 2018. doi: 10.1145/3149457.3149478

For details, read the comments in tiling.cpp.

License
------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Note that implementation of algorithms can be copyrighted, but algorithms
themselves cannot. Thus, if you simply read the code to understand how
its algorithms work, then write your own implementation without copying
any code, it's not covered by GPL.

But seriously, don't do it for now.

The author is still in the process of trying to understand the tiling
techniques, so the code in its current form is broken, incorrect, and
ugly, so don't use it. After development is finished, I plan to write
a blog post to explain how everything works, which should happen before
the end of 2023, be patient. But if you still decide to read it, you're
on your own.
