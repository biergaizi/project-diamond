#include "narray3d.hpp"
#include <ginac/ginac.h>

void updateVoltageRange(
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& vv,
	const NArray3D<GiNaC::ex>& vi,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
);

void updateCurrentRange(
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& ii,
	const NArray3D<GiNaC::ex>& iv,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
);
