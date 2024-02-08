Project Diamond
====================

This repository contains experimental code for 3D FDTD domain decomposition
using parallelogram and diamond tiling.

This is the first draft of the gode, so the code in its current form
contains numerous problems, including broken, incorrect, misnamed,
ugly concepts and code, so it's *not* recommend to make use of the
code in its current form. The eventual plan is to rewrite it and
integrate it into openEMS, a GPU version is also planned.

For a detailed explanation on its theory of operation, please refer
to the article [*Temporal Tiling: The Key to Fast FDTD Simulations, Explained*](https://github.com/thliebig/openEMS-Project/discussions/154#discussioncomment-8183199). To minimize wasted time, do not
attempt to read the code unless you've read the article and the cited
main paper.

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

openEMS
------------

This is part of my simulation speed improvement effort for the openEMS
project, which is currently ongoing. I kindly request any reader to avoid
attempting to do the same for now to avoid duplicated work.

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
