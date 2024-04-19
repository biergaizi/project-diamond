#include <cstdint>
#include "array3d.hpp"

void checkVoltageKernel(
	Array3D<uint32_t>& volt,
	Array3D<uint32_t>& curr,
	uint32_t i, uint32_t j, uint32_t k
);

void checkCurrentKernel(
	Array3D<uint32_t>& curr,
	Array3D<uint32_t>& volt,
	uint32_t i, uint32_t j, uint32_t k
);

void checkVoltageRange(
	Array3D<uint32_t>& volt,
	Array3D<uint32_t>& curr,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
);

void checkCurrentRange(
	Array3D<uint32_t>& curr,
	Array3D<uint32_t>& volt,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
);
