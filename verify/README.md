Verify: Symbolic Verification of Tiling Correctness
=========================================================

This tool performs symbolic verification of FDTD tiling plans.

In symbolic verification, the floating-point values of the electromagnetic
field are replaced by symbolic mathematical variables from the algebra
system GiNaC. By running the simulation code for naive and tiled FDTD
algorithm and comparing the resulted algebraic expressions, one can ensure
the tiling algorithm is mathematical correct.

Because the size of algebraic expressions grows rapidly, it can only be used
at a very small grid size with short simulation timesteps. For a 70,70,70
grid with a tile size of timestep size of 20t,20t,20p, 64 GiB of memory
is required. Thus, don't even think about trying more timesteps unless more
memory is available.

Still, this technique is extremely valuable for debugging since it can report
both the location of the bug, and show the mismatch as algebra expressions to
point out exactly which terms are wrong or missing. Also, simulation of all
timesteps is not required for verification, since when correct results are
obtained by applying the tiling plan *once*, ideally it must also be true
when the same tiling plan is applied multiple times.

Usage
---------

    ./verify: Symbolic Verification of Tiling Correctness
    
    Usage: ./verify [OPTION]
       --grid-size		-g	i,j,k			(e.g: 400,400,400)
       --tile-size		-t	ip,jp,kp/kt		(e.g: 20p,20p,20p or 20p,20p,20t)
       --tile-height	-h	halfTimesteps		(e.g: 18)
       --total-timesteps	-n	timesteps		(defafult: 100)
       --dump		-d	dump traces for debugging	(default: no)
    
    Note: Parallelogram tiling uses suffix "p", trapezoid tiling uses suffix "t".
    Note: Symbolic verification requires extreme memory usage. 64 GiB PC is
    required for a 70,70,70 grid with timestep size of 20, don't even think
    about trying more timesteps unless more memory is available.
