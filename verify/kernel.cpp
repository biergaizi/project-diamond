#include <format>
#include <ginac/ginac.h>
#include "narray3d.hpp"

inline void updateVoltageKernel(
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& vv,
	const NArray3D<GiNaC::ex>& vi,
	size_t i, size_t j, size_t k
)
{
	size_t prev_i = i > 0 ? i - 1 : 0;
	size_t prev_j = j > 0 ? j - 1 : 0;
	size_t prev_k = k > 0 ? k - 1 : 0;

	// 3 FP32 loads
	GiNaC::ex volt0 = volt(i, j, k, 0);
	GiNaC::ex volt1 = volt(i, j, k, 1);
	GiNaC::ex volt2 = volt(i, j, k, 2);

	// 3 FP32 loads
	GiNaC::ex vv0 = vv(i, j, k, 0);
	GiNaC::ex vv1 = vv(i, j, k, 1);
	GiNaC::ex vv2 = vv(i, j, k, 2);

	// 3 FP32 loads
	GiNaC::ex vi0 = vi(i, j, k, 0);
	GiNaC::ex vi1 = vi(i, j, k, 1);
	GiNaC::ex vi2 = vi(i, j, k, 2);

	// 9 FP32 loads
	GiNaC::ex curr0_ci_cj_ck = curr(i,      j,      k     , 0);
	GiNaC::ex curr1_ci_cj_ck = curr(i,      j,      k     , 1);
	GiNaC::ex curr2_ci_cj_ck = curr(i,      j,      k     , 2);
	GiNaC::ex curr0_ci_cj_pk = curr(i,      j,      prev_k, 0);
	GiNaC::ex curr1_ci_cj_pk = curr(i,      j,      prev_k, 1);
	GiNaC::ex curr0_ci_pj_ck = curr(i,      prev_j, k     , 0);
	GiNaC::ex curr2_ci_pj_ck = curr(i,      prev_j, k     , 2);
	GiNaC::ex curr1_pi_cj_ck = curr(prev_i, j,      k     , 1);
	GiNaC::ex curr2_pi_cj_ck = curr(prev_i, j,      k     , 2);

	// 6 FLOPs, for x polarization
	volt0 *= vv0;
	volt0 +=
		vi0 * (
			curr2_ci_cj_ck -
			curr2_ci_pj_ck -
			curr1_ci_cj_ck +
			curr1_ci_cj_pk
		);

	// 6 FLOPs, for y polarization
	volt1 *= vv1;
	volt1 +=
		vi1 * (
			curr0_ci_cj_ck -
			curr0_ci_cj_pk -
			curr2_ci_cj_ck +
			curr2_pi_cj_ck
		);

	// 6 FLOPs, for z polarization
	volt2 *= vv2;
	volt2 +=
		vi2 * (
			curr1_ci_cj_ck -
			curr1_pi_cj_ck -
			curr0_ci_cj_ck +
			curr0_ci_pj_ck
		);

	// 3 FP32 stores
	volt(i, j, k, 0) = volt0;
	volt(i, j, k, 1) = volt1;
	volt(i, j, k, 2) = volt2;
}

inline void updateCurrentKernel(
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& ii,
	const NArray3D<GiNaC::ex>& iv,
	size_t i, size_t j, size_t k
)
{
	// 3 FP32 loads
	GiNaC::ex curr0 = curr(i, j, k, 0);
	GiNaC::ex curr1 = curr(i, j, k, 1);
	GiNaC::ex curr2 = curr(i, j, k, 2);

	// 3 FP32 loads
	GiNaC::ex ii0 = ii(i, j, k, 0);
	GiNaC::ex ii1 = ii(i, j, k, 1);
	GiNaC::ex ii2 = ii(i, j, k, 2);

	// 3 FP32 loads
	GiNaC::ex iv0 = iv(i, j, k, 0);
	GiNaC::ex iv1 = iv(i, j, k, 1);
	GiNaC::ex iv2 = iv(i, j, k, 2);

	// 9 FP32 loads
	GiNaC::ex volt0_ci_cj_ck = volt(i,   j,   k  , 0);
	GiNaC::ex volt1_ci_cj_ck = volt(i,   j,   k  , 1);
	GiNaC::ex volt2_ci_cj_ck = volt(i,   j,   k  , 2);
	GiNaC::ex volt0_ci_cj_nk = volt(i,   j,   k+1, 0);
	GiNaC::ex volt1_ci_cj_nk = volt(i,   j,   k+1, 1);
	GiNaC::ex volt0_ci_nj_ck = volt(i,   j+1, k  , 0);
	GiNaC::ex volt2_ci_nj_ck = volt(i,   j+1, k  , 2);
	GiNaC::ex volt1_ni_cj_ck = volt(i+1, j,   k  , 1);
	GiNaC::ex volt2_ni_cj_ck = volt(i+1, j,   k  , 2);

	// 6 FLOPs, for x polarization
	curr0 *= ii0;
	curr0 +=
		iv0 * (
			volt2_ci_cj_ck -
			volt2_ci_nj_ck -
			volt1_ci_cj_ck +
			volt1_ci_cj_nk
		);

	// 6 FLOPs, for y polarization
	curr1 *= ii1;
	curr1 +=
		iv1 * (
			volt0_ci_cj_ck -
			volt0_ci_cj_nk -
			volt2_ci_cj_ck +
			volt2_ni_cj_ck
		);

	// 6 FLOPs, for z polarization
	curr2 *= ii2;
	curr2 +=
		iv2 * (
			volt1_ci_cj_ck -
			volt1_ni_cj_ck -
			volt0_ci_cj_ck +
			volt0_ci_nj_ck
		);

	// 3 FP32 stores
	curr(i, j, k, 0) = curr0;
	curr(i, j, k, 1) = curr1;
	curr(i, j, k, 2) = curr2;
}

void updateVoltageRange(
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& vv,
	const NArray3D<GiNaC::ex>& vi,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
)
{
	if (debug) {
		std::cerr << std::format(
			"\tupdateing volt({}, {}, {}) - volt({}, {}, {})\n",
			first[0], first[1], first[2],
			last[0],  last[1],  last[2]
		);
	}

	for (size_t i = first[0]; i <= last[0]; i++) {
		for (size_t j = first[1]; j <= last[1]; j++) {
			for (size_t k = first[2]; k <= last[2]; k++) {
				updateVoltageKernel(volt, curr, vv, vi, i, j, k);
			}
		}
	}
}

void updateCurrentRange(
	const NArray3D<GiNaC::ex>& curr,
	const NArray3D<GiNaC::ex>& volt,
	const NArray3D<GiNaC::ex>& ii,
	const NArray3D<GiNaC::ex>& iv,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
)
{
	if (debug) {
		std::cerr << std::format(
			"\tupdateing curr({}, {}, {}) - curr({}, {}, {})\n",
			first[0], first[1], first[2],
			last[0],  last[1],  last[2]
		);
	}

	for (size_t i = first[0]; i <= last[0]; i++) {
		for (size_t j = first[1]; j <= last[1]; j++) {
			for (size_t k = first[2]; k <= last[2]; k++) {
				updateCurrentKernel(volt, curr, ii, iv, i, j, k);
			}
		}
	}
}
