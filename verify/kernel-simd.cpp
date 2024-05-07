#include <cassert>
#include <format>
#include <ginac/ginac.h>

#include "narray3d.hpp"
#include "simd.hpp"

static const size_t veclen = 4;

inline static void updateVoltageScalarKernel(
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& vv,
	const NArray3D<Simd<GiNaC::ex, 4>>& vi,
	size_t i,
	size_t j,
	size_t first_k,
	size_t last_k
)
{
	size_t pi = i > 0 ? i - 1 : 0;
	size_t pj = j > 0 ? j - 1 : 0;

	for (size_t k = first_k; k <= last_k; k++) {
		// 3 FP32 loads
		size_t vk = k / veclen;
		size_t vk_offset = k % veclen;

		GiNaC::ex volt0_ci_cj_ck = volt(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex volt1_ci_cj_ck = volt(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex volt2_ci_cj_ck = volt(i, j, vk, 2).elem[vk_offset];

		// 3 FP32 loads
		GiNaC::ex vv0_ci_cj_ck = vv(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex vv1_ci_cj_ck = vv(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex vv2_ci_cj_ck = vv(i, j, vk, 2).elem[vk_offset];

		// 3 FP32 loads
		GiNaC::ex vi0_ci_cj_ck = vi(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex vi1_ci_cj_ck = vi(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex vi2_ci_cj_ck = vi(i, j, vk, 2).elem[vk_offset];

		// 9 FP32 loads
		//assert(k > 0);
		//size_t pk = k - 1;
		size_t pk = k > 0 ? k - 1 : 0;
		size_t vpk = pk / veclen;
		size_t vpk_offset = pk % veclen;

		GiNaC::ex curr0_ci_cj_ck = curr(i,  j,  vk,  0).elem[vk_offset];
		GiNaC::ex curr1_ci_cj_ck = curr(i,  j,  vk,  1).elem[vk_offset];
		GiNaC::ex curr2_ci_cj_ck = curr(i,  j,  vk,  2).elem[vk_offset];
		GiNaC::ex curr0_ci_cj_pk = curr(i,  j,  vpk, 0).elem[vpk_offset];
		GiNaC::ex curr1_ci_cj_pk = curr(i,  j,  vpk, 1).elem[vpk_offset];
		GiNaC::ex curr0_ci_pj_ck = curr(i,  pj, vk,  0).elem[vk_offset];
		GiNaC::ex curr2_ci_pj_ck = curr(i,  pj, vk,  2).elem[vk_offset];
		GiNaC::ex curr1_pi_cj_ck = curr(pi, j,  vk,  1).elem[vk_offset];
		GiNaC::ex curr2_pi_cj_ck = curr(pi, j,  vk,  2).elem[vk_offset];

		// 6 FLOPs, for x polarization
		volt0_ci_cj_ck *= vv0_ci_cj_ck;
		volt0_ci_cj_ck +=
			vi0_ci_cj_ck * (
				curr2_ci_cj_ck -
				curr2_ci_pj_ck -
				curr1_ci_cj_ck +
				curr1_ci_cj_pk
			);

		// 6 FLOPs, for y polarization
		volt1_ci_cj_ck *= vv1_ci_cj_ck;
		volt1_ci_cj_ck +=
			vi1_ci_cj_ck * (
				curr0_ci_cj_ck -
				curr0_ci_cj_pk -
				curr2_ci_cj_ck +
				curr2_pi_cj_ck
			);

		// 6 FLOPs, for z polarization
		volt2_ci_cj_ck *= vv2_ci_cj_ck;
		volt2_ci_cj_ck +=
			vi2_ci_cj_ck * (
				curr1_ci_cj_ck -
				curr1_pi_cj_ck -
				curr0_ci_cj_ck +
				curr0_ci_pj_ck
			);

		volt(i, j, vk, 0).elem[vk_offset] = volt0_ci_cj_ck;
		volt(i, j, vk, 1).elem[vk_offset] = volt1_ci_cj_ck;
		volt(i, j, vk, 2).elem[vk_offset] = volt2_ci_cj_ck;
	}
}

template <bool boundary>
inline static void updateVoltageVectorKernel(
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& vv,
	const NArray3D<Simd<GiNaC::ex, 4>>& vi,
	size_t i,
	size_t j,
	size_t first_k,
	size_t last_k
)
{
	size_t pi = i > 0 ? i - 1 : 0;
	size_t pj = j > 0 ? j - 1 : 0;

	size_t first_vk = first_k / veclen;
	size_t last_vk = last_k / veclen;

	for (size_t vk = first_vk; vk <= last_vk; vk += 1) {
		// 12 (3 x 4) FP32 loads
		Simd<GiNaC::ex, 4> volt0_ci_cj_ck = volt(i, j, vk, 0);
		Simd<GiNaC::ex, 4> volt1_ci_cj_ck = volt(i, j, vk, 1);
		Simd<GiNaC::ex, 4> volt2_ci_cj_ck = volt(i, j, vk, 2);

		// 12 (3 x 4) FP32 loads
		Simd<GiNaC::ex, 4> curr0_ci_cj_ck = curr(i, j, vk, 0);
		Simd<GiNaC::ex, 4> curr1_ci_cj_ck = curr(i, j, vk, 1);
		Simd<GiNaC::ex, 4> curr2_ci_cj_ck = curr(i, j, vk, 2);

		// 16 (4 x 4) FP32 loads
		Simd<GiNaC::ex, 4> curr0_ci_pj_ck = curr(i,  pj, vk, 0);
		Simd<GiNaC::ex, 4> curr2_ci_pj_ck = curr(i,  pj, vk, 2);
		Simd<GiNaC::ex, 4> curr1_pi_cj_ck = curr(pi, j,  vk, 1);
		Simd<GiNaC::ex, 4> curr2_pi_cj_ck = curr(pi, j,  vk, 2);

		// 2 misaligned FP32 loads
		Simd<GiNaC::ex, 4> curr0_ci_cj_pk;
		curr0_ci_cj_pk.elem[1] = curr0_ci_cj_ck.elem[0];
		curr0_ci_cj_pk.elem[2] = curr0_ci_cj_ck.elem[1];
		curr0_ci_cj_pk.elem[3] = curr0_ci_cj_ck.elem[2];

		Simd<GiNaC::ex, 4> curr1_ci_cj_pk;
		curr1_ci_cj_pk.elem[1] = curr1_ci_cj_ck.elem[0];
		curr1_ci_cj_pk.elem[2] = curr1_ci_cj_ck.elem[1];
		curr1_ci_cj_pk.elem[3] = curr1_ci_cj_ck.elem[2];

		if constexpr (boundary) {
			curr0_ci_cj_pk.elem[0] = curr0_ci_cj_ck.elem[0];
			curr1_ci_cj_pk.elem[0] = curr1_ci_cj_ck.elem[0];
		}
		else {
			curr0_ci_cj_pk.elem[0] = curr(i, j, vk - 1, 0).elem[3];
			curr1_ci_cj_pk.elem[0] = curr(i, j, vk - 1, 1).elem[3];
		}

		// 24 (6 x 4) FP32 loads
		Simd<GiNaC::ex, 4> vv0_ci_cj_ck = vv(i, j, vk, 0);
		Simd<GiNaC::ex, 4> vv1_ci_cj_ck = vv(i, j, vk, 1);
		Simd<GiNaC::ex, 4> vv2_ci_cj_ck = vv(i, j, vk, 2);
		Simd<GiNaC::ex, 4> vi0_ci_cj_ck = vi(i, j, vk, 0);
		Simd<GiNaC::ex, 4> vi1_ci_cj_ck = vi(i, j, vk, 1);
		Simd<GiNaC::ex, 4> vi2_ci_cj_ck = vi(i, j, vk, 2);

		// x-polarization
		volt0_ci_cj_ck *= vv0_ci_cj_ck;
		volt0_ci_cj_ck +=
			vi0_ci_cj_ck * (
				curr2_ci_cj_ck -
				curr2_ci_pj_ck -
				curr1_ci_cj_ck +
				curr1_ci_cj_pk
			);

		// y-polarization
		volt1_ci_cj_ck *= vv1_ci_cj_ck;
		volt1_ci_cj_ck +=
			vi1_ci_cj_ck * (
				curr0_ci_cj_ck -
				curr0_ci_cj_pk -
				curr2_ci_cj_ck +
				curr2_pi_cj_ck
			);

		// z-polarization
		volt2_ci_cj_ck *= vv2_ci_cj_ck;
		volt2_ci_cj_ck +=
			vi2_ci_cj_ck * (
				curr1_ci_cj_ck -
				curr1_pi_cj_ck -
				curr0_ci_cj_ck +
				curr0_ci_pj_ck
			);

		// 12 (3 x 4) FP32 stores
		volt(i, j, vk, 0) = volt0_ci_cj_ck;
		volt(i, j, vk, 1) = volt1_ci_cj_ck;
		volt(i, j, vk, 2) = volt2_ci_cj_ck;
	}
}

void updateVoltageRange(
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& vv,
	const NArray3D<Simd<GiNaC::ex, 4>>& vi,
	const std::array<size_t, 3> first,
	const std::array<size_t, 3> last,
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

	// The K dimension is vectorized but the loop range (first_k, last_k)
	// is often at the middle of some vectors. Thus, we split the inner
	// loop into three ranges: head, body, tail. The body is exactly aligned
	// to a vector, but the misaligned head and tail need special treatments.
	std::pair<size_t, size_t> headRange, bodyRange, tailRange;
	if (first[2] % veclen == 0) {
		headRange = {0, 0};
		bodyRange.first = first[2];
	}
	else {
		headRange.first = first[2];
		bodyRange.first = first[2] + (veclen - (first[2] % veclen));
		headRange.second = bodyRange.first - 1;
	}

	if ((last[2] + 1) % veclen == 0) {
		tailRange = {0, 0};
		bodyRange.second = last[2];
	}
	else {
		bodyRange.second = last[2] - (last[2] % veclen) - 1;
		tailRange.first = bodyRange.second + 1;
		tailRange.second = last[2];
	}

	for (size_t i = first[0]; i <= last[0]; i++) {
		for (size_t j = first[1]; j <= last[1]; j++) {
			// prologue: head
			if (headRange.first > 0) {
				updateVoltageScalarKernel(
					volt, curr, vv, vi,
					i, j,
					headRange.first, headRange.second
				);
			}

			// body
			if (bodyRange.first == 0) {
				updateVoltageVectorKernel<true>(
					volt, curr, vv, vi,
					i, j,
					0, veclen - 1
				);
				updateVoltageVectorKernel<false>(
					volt, curr, vv, vi,
					i, j,
					veclen, bodyRange.second
				);
			}
			else {
				updateVoltageVectorKernel<false>(
					volt, curr, vv, vi,
					i, j,
					bodyRange.first, bodyRange.second
				);
			}

			// epilogue: tail
			if (tailRange.first > 0) {
				updateVoltageScalarKernel(
					volt, curr, vv, vi,
					i, j,
					tailRange.first, tailRange.second
				);
			}
		}
	}
}

inline static void updateCurrentScalarKernel(
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& ii,
	const NArray3D<Simd<GiNaC::ex, 4>>& iv,
	size_t i,
	size_t j,
	size_t first_k,
	size_t last_k
)
{
	for (size_t k = first_k; k <= last_k; k++) {
		// 3 FP32 loads
		size_t vk = k / veclen;
		size_t vk_offset = k % veclen;

		GiNaC::ex curr0_ci_cj_ck = curr(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex curr1_ci_cj_ck = curr(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex curr2_ci_cj_ck = curr(i, j, vk, 2).elem[vk_offset];

		// 3 FP32 loads
		GiNaC::ex ii0_ci_cj_ck = ii(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex ii1_ci_cj_ck = ii(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex ii2_ci_cj_ck = ii(i, j, vk, 2).elem[vk_offset];

		// 3 FP32 loads
		GiNaC::ex iv0_ci_cj_ck = iv(i, j, vk, 0).elem[vk_offset];
		GiNaC::ex iv1_ci_cj_ck = iv(i, j, vk, 1).elem[vk_offset];
		GiNaC::ex iv2_ci_cj_ck = iv(i, j, vk, 2).elem[vk_offset];

		// 9 FP32 loads
		size_t nk = k + 1;
		size_t vnk = nk / veclen;
		size_t vnk_offset = nk % veclen;

		GiNaC::ex volt0_ci_cj_ck = volt(i,   j,   vk , 0).elem[vk_offset];
		GiNaC::ex volt1_ci_cj_ck = volt(i,   j,   vk , 1).elem[vk_offset];
		GiNaC::ex volt0_ci_cj_nk = volt(i,   j,   vnk, 0).elem[vnk_offset];
		GiNaC::ex volt1_ci_cj_nk = volt(i,   j,   vnk, 1).elem[vnk_offset];
		GiNaC::ex volt2_ci_cj_ck = volt(i,   j,   vk , 2).elem[vk_offset];
		GiNaC::ex volt0_ci_nj_ck = volt(i,   j+1, vk , 0).elem[vk_offset];
		GiNaC::ex volt2_ci_nj_ck = volt(i,   j+1, vk , 2).elem[vk_offset];
		GiNaC::ex volt1_ni_cj_ck = volt(i+1, j,   vk , 1).elem[vk_offset];
		GiNaC::ex volt2_ni_cj_ck = volt(i+1, j,   vk , 2).elem[vk_offset];

		// 6 FLOPs, for x polarization
		curr0_ci_cj_ck *= ii0_ci_cj_ck;
		curr0_ci_cj_ck +=
			iv0_ci_cj_ck * (
				volt2_ci_cj_ck -
				volt2_ci_nj_ck -
				volt1_ci_cj_ck +
				volt1_ci_cj_nk
			);

		// 6 FLOPs, for y polarization
		curr1_ci_cj_ck *= ii1_ci_cj_ck;
		curr1_ci_cj_ck +=
			iv1_ci_cj_ck * (
				volt0_ci_cj_ck -
				volt0_ci_cj_nk -
				volt2_ci_cj_ck +
				volt2_ni_cj_ck
			);

		// 6 FLOPs, for z polarization
		curr2_ci_cj_ck *= ii2_ci_cj_ck;
		curr2_ci_cj_ck +=
			iv2_ci_cj_ck * (
				volt1_ci_cj_ck -
				volt1_ni_cj_ck -
				volt0_ci_cj_ck +
				volt0_ci_nj_ck
			);

		// 3 FP32 stores
		curr(i, j, vk, 0).elem[vk_offset] = curr0_ci_cj_ck;
		curr(i, j, vk, 1).elem[vk_offset] = curr1_ci_cj_ck;
		curr(i, j, vk, 2).elem[vk_offset] = curr2_ci_cj_ck;
	}
}

template <bool boundary>
inline static void updateCurrentVectorKernel(
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& ii,
	const NArray3D<Simd<GiNaC::ex, 4>>& iv,
	size_t i,
	size_t j,
	size_t first_k,
	size_t last_k
)
{
	size_t first_vk = first_k / veclen;
	size_t last_vk = last_k / veclen;

	for (size_t vk = first_vk; vk <= last_vk; vk += 1) {
		// 12 (3 x 4) FP32 loads
		Simd<GiNaC::ex, 4> curr0_ci_cj_ck = curr(i,     j,     vk, 0);
		Simd<GiNaC::ex, 4> curr1_ci_cj_ck = curr(i,     j,     vk, 1);
		Simd<GiNaC::ex, 4> curr2_ci_cj_ck = curr(i,     j,     vk, 2);

		// 12 (3 x 4) FP32 loads
		Simd<GiNaC::ex, 4> volt0_ci_cj_ck = volt(i,     j,     vk, 0);
		Simd<GiNaC::ex, 4> volt1_ci_cj_ck = volt(i,     j,     vk, 1);
		Simd<GiNaC::ex, 4> volt2_ci_cj_ck = volt(i,     j,     vk, 2);

		// 16 (4 x 4) FP32 loads
		Simd<GiNaC::ex, 4> volt0_ci_nj_ck = volt(i,     j + 1, vk, 0);
		Simd<GiNaC::ex, 4> volt2_ci_nj_ck = volt(i,     j + 1, vk, 2);
		Simd<GiNaC::ex, 4> volt1_ni_cj_ck = volt(i + 1, j,     vk, 1);
		Simd<GiNaC::ex, 4> volt2_ni_cj_ck = volt(i + 1, j,     vk, 2);

		// 2 misaligned FP32 loads
		Simd<GiNaC::ex, 4> volt0_ci_cj_nk;
		volt0_ci_cj_nk.elem[0] = volt0_ci_cj_ck.elem[1];
		volt0_ci_cj_nk.elem[1] = volt0_ci_cj_ck.elem[2];
		volt0_ci_cj_nk.elem[2] = volt0_ci_cj_ck.elem[3];

		Simd<GiNaC::ex, 4> volt1_ci_cj_nk;
		volt1_ci_cj_nk.elem[0] = volt1_ci_cj_ck.elem[1];
		volt1_ci_cj_nk.elem[1] = volt1_ci_cj_ck.elem[2];
		volt1_ci_cj_nk.elem[2] = volt1_ci_cj_ck.elem[3];

		if constexpr (boundary) {
			volt0_ci_cj_nk.elem[3] = 0;
			volt1_ci_cj_nk.elem[3] = 0;
		}
		else {
			volt0_ci_cj_nk.elem[3] = volt(i, j, vk + 1, 0).elem[0];
			volt1_ci_cj_nk.elem[3] = volt(i, j, vk + 1, 1).elem[0];
		}

		// 24 (6 x 4) FP32 loads
		Simd<GiNaC::ex, 4> ii0_ci_cj_ck = ii(i, j, vk, 0);
		Simd<GiNaC::ex, 4> ii1_ci_cj_ck = ii(i, j, vk, 1);
		Simd<GiNaC::ex, 4> ii2_ci_cj_ck = ii(i, j, vk, 2);
		Simd<GiNaC::ex, 4> iv0_ci_cj_ck = iv(i, j, vk, 0);
		Simd<GiNaC::ex, 4> iv1_ci_cj_ck = iv(i, j, vk, 1);
		Simd<GiNaC::ex, 4> iv2_ci_cj_ck = iv(i, j, vk, 2);

		// x-polarization
		curr0_ci_cj_ck *= ii0_ci_cj_ck;
		curr0_ci_cj_ck +=
			iv0_ci_cj_ck * (
				volt2_ci_cj_ck -
				volt2_ci_nj_ck -
				volt1_ci_cj_ck +
				volt1_ci_cj_nk
			);

		// y-polarization
		curr1_ci_cj_ck *= ii1_ci_cj_ck;
		curr1_ci_cj_ck +=
			iv1_ci_cj_ck * (
				volt0_ci_cj_ck -
				volt0_ci_cj_nk -
				volt2_ci_cj_ck +
				volt2_ni_cj_ck
			);

		// z-polarization
		curr2_ci_cj_ck *= ii2_ci_cj_ck;
		curr2_ci_cj_ck +=
			iv2_ci_cj_ck * (
				volt1_ci_cj_ck -
				volt1_ni_cj_ck -
				volt0_ci_cj_ck +
				volt0_ci_nj_ck
			);

		// 12 (3 x 4) FP32 stores
		curr(i, j, vk, 0) = curr0_ci_cj_ck;
		curr(i, j, vk, 1) = curr1_ci_cj_ck;
		curr(i, j, vk, 2) = curr2_ci_cj_ck;
	}
}

void updateCurrentRange(
	const NArray3D<Simd<GiNaC::ex, 4>>& curr,
	const NArray3D<Simd<GiNaC::ex, 4>>& volt,
	const NArray3D<Simd<GiNaC::ex, 4>>& ii,
	const NArray3D<Simd<GiNaC::ex, 4>>& iv,
	const std::array<size_t, 3> first,
	const std::array<size_t, 3> last,
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

	// The K dimension is vectorized but the loop range (first_k, last_k)
	// is often at the middle of some vectors. Thus, we split the inner
	// loop into three ranges: head, body, tail. The body is exactly aligned
	// to a vector, but the misaligned head and tail need special treatments.
	std::pair<uint32_t, uint32_t> headRange, bodyRange, tailRange;
	if (first[2] % veclen == 0) {
		headRange = {0, 0};
		bodyRange.first = first[2];
	}
	else {
		headRange.first = first[2];
		bodyRange.first = first[2] + (veclen - (first[2] % veclen));
		headRange.second = bodyRange.first - 1;
	}

	if ((last[2] + 1) % veclen == 0) {
		tailRange = {0, 0};
		bodyRange.second = last[2];
	}
	else {
		bodyRange.second = last[2] - (last[2] % veclen) - 1;
		tailRange.first = bodyRange.second + 1;
		tailRange.second = last[2];
	}

	for (uint32_t i = first[0]; i <= last[0]; i++) {
		for (uint32_t j = first[1]; j <= last[1]; j++) {
			// prologue: head
			if (headRange.first > 0) {
				updateCurrentScalarKernel(
					curr, volt, ii, iv,
					i, j,
					headRange.first, headRange.second
				);
			}

			// body
			if (bodyRange.second == volt.k() * veclen - 1) {
				updateCurrentVectorKernel<false>(
					curr, curr, ii, iv,
					i, j,
					bodyRange.first, bodyRange.second - veclen
				);
				updateCurrentVectorKernel<true>(
					curr, volt, ii, iv,
					i, j,
					bodyRange.second - veclen + 1, bodyRange.second
				);
			}
			else {
				updateCurrentVectorKernel<false>(
					curr, volt, ii, iv,
					i, j,
					bodyRange.first, bodyRange.second
				);
			}

			// epilogue: tail
			if (tailRange.first > 0) {
				updateCurrentScalarKernel(
					curr, volt, ii, iv,
					i, j,
					tailRange.first, tailRange.second
				);
			}
		}
	}
}
