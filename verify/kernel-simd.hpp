#include <ginac/ginac.h>
#include "narray3d.hpp"

void updateVoltageRange(
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& vv,
	const NArray3D<Simd<GiNaC::ex, 4>>& vi,
	const std::array<size_t, 3> first,
	const std::array<size_t, 3> last,
	bool debug
);

void updateCurrentRange(
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& ii,
	const NArray3D<Simd<GiNaC::ex, 4>>& iv,
	const std::array<size_t, 3> first,
	const std::array<size_t, 3> last,
	bool debug
);
