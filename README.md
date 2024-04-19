Project Diamond
====================

This repository contains experimental code for 3D FDTD domain decomposition
using parallelogram and trapezoid tiling.

## Introduction

### FDTD

In microwave and high-speed digital electronics engineering, one often
needs to directly simulate electromagnetic waves by numerically solving
Maxwell's equations using the Finite-Difference Time-Domain (FDTD)
algorithm.

Unfortunately this algorithm loads a large number of cells into CPU's
last-level cache, performs only a few multiplications and adds, before
writing them back to memory. Thus, the FDTD kernel has an extremely low
arithmetic intensity (around 0.25 FLOPS/bytes) and is bottlenecked by
memory bandwidth, simulation speed is extremely slow.

A practical FDTD model may contain at least thousands of timesteps with
over 1 million cells with a total memory traffic measured in TiB, but a
desktop computer only has 40 GB/s of memory bandwidth. As a result,
FDTD simulations using the textbook algorithm are painfully slow since
they only runs at ~1% of the CPU's peak floating-point performance.

### Loop Tiling and Time Skewing

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

I've also explained the algorithms in details in the article:

* [Temporal Tiling: The Key to Fast FDTD Simulations, Explained](https://github.com/thliebig/openEMS-Project/discussions/154#discussioncomment-8183199).

To minimize wasted time, do not attempt to read the code unless you've read the
article and the cited main paper.

## Organization of the Code

1. Directory `tiling/` contains the main implementation of the tiling plan
generator, which produces a data structure that describes how the 3D space
should be iterated.

   Higher performance is probably achievable by hardcoding specialized loops
or generating code on-the-fly, but I found doing it this way is nearly
impossible to understand, let alone debugging it. By separating tiling plan
and logic, testability and understandability is greatly improved.

2. Directory `utils/` contains some test utility to help understanding
or debugging the tiling algorithm.

    * Tool `demo` visualizes the tiling plan as an ASCII diagram.
    * Tool `speedup` calculates the theoretical memory traffic saving by
    tiling.
    * Tool `shapes` counts the total unique 3D shapes in a tiling plan.

3. Directory `sanity/` contains a quick-and-dirty "sanity check" tool of
tiling correctness by checking whethe there are violations of timestep
dependencies. The result is not reliable so passing the sanity check is
no guarantee of correctness, but failing the check means there exists
serious problems, so it's a useful first-pass checker.

4. Directory `verify/` contains a full symbolic verification tool to check
whether the tiling plan is mathematically correct using the GiNaC algebra
library. Passing this verification indicates strong confidence that the
tiling algorithm is correct, and is also an extremely useful debugging aid as
mistakes can be demostrated algebraically. However, due to extreme high memory
requirements and extremely low performance due to the nature of symbolic
computation, it's only used as the final step of the development, and can only
verify very small grid and timestep sizes.

## Limitations

1. Unlike what is suggested by the name *project diamond*, Only parallelogram
and trapezoid tilings are implemented. It was named *diamond* because in
some papers, trapezoid and diamond tilings are used interchangeably, but
diamond tiling is in fact a further optimized version, they are not equivalent.
For details, see my article *Temporal Tiling: The Key to Fast FDTD Simulations,
Explained* linked above. However, it was realized after the first version
of this project was published...

2. Only Trapezoid-Trapezoid-Parallelogram and Trapezoid-Trapezoid-Trapezoid
tilings are supported, other combinations are unsupported due to lack of
practical values, but it should be trivial to add them.

3. The findings of the research paper by *Fukaya, T., & Iwashita, T.* are
replicated, performance is doubled after applying temporal tiling, and
the measured memory traffic is significantly reduced. However, due to suboptimal
memory access patterns, the tiled version is still unable to utilize the
entire L3 cache bandwidth. The simulation should be 500% to 1000% as fast, not
just 200% as fast. However, improving data layout is non-trivial due to the
overlapped nature of tiling plans. Another idea is that by further tiling at
the level of L2, L1, SIMD vectors, or registers, greater speedup should be
possible. Still, this is easier said than done.

4. Alternatively, perhaps the further development of trapezoid or diamond tiling
is a dead end, since reseachers from Russian Keldysh Institute of Applied
Mathematics have reported that the *DiamondTorre* and *DiamondCandy* algorithms
have much higher performance (again, see my referenced article). However,
both are more difficult to understand than trapezoid or diamond tiling, which
requires a good intuition in 3D and 4D geometry. In comparison, trapezoid and
diamond tilings are old and classic algorithms well-known in HPC, and
understandable in 2D.

## Real-World Application

To the author's best knowledge, none of the popular academic FDTD field
solvers such as openEMS, or MIT's MEEP, or gprMax, has implemented FDTD
using an optimized algorithm - all use the textbook algorithm. This
represents a huge wasted opportunity.

Currently, I'm working on improving the simulation speed of openEMS,
tiling is one part of this still-ongoing multi-year project. Thus, 
I kindly request any reader to avoid attempting to do the same for now
to avoid duplicated work.

License
------------

    BSD Zero Clause License

    Permission to use, copy, modify, and/or distribute this software for any
    purpose with or without fee is hereby granted.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
    REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
    INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
    LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
    OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
    PERFORMANCE OF THIS SOFTWARE.
