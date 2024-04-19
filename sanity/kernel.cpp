#include <iostream>
#include <stdexcept>
#include <format>
#include "kernel.hpp"

void checkVoltageKernel(
	Array3D<uint32_t>& volt,
	Array3D<uint32_t>& curr,
	uint32_t i, uint32_t j, uint32_t k
)
{
	uint32_t prev_i = i > 0 ? i - 1 : 0;
	uint32_t prev_j = j > 0 ? j - 1 : 0;
	uint32_t prev_k = k > 0 ? k - 1 : 0;

	size_t volt_ci_cj_ck_ts = volt(i, j, k);

	size_t curr_ci_cj_ck_ts = curr(i, j, k);
	size_t curr_ci_cj_pk_ts = curr(i, j, prev_k);
	size_t curr_ci_pj_ck_ts = curr(i, prev_j, k);
	size_t curr_pi_cj_ck_ts = curr(prev_i, j, k);

	volt_ci_cj_ck_ts++;
	volt(i, j, k) = volt_ci_cj_ck_ts;

	if (prev_i == 0 || prev_j == 0 || prev_k == 0) {
		// the all-zero volt boundary is always up-to-date,
		// don't check
		return;
	}
	if (i == curr.i() - 1 || j == curr.j() - 1 || k == curr.k() - 1) {
		// the all-zero curr boundary is always up-to-date,
		// don't check
		return;
	}

	if (curr_ci_cj_ck_ts == curr_ci_cj_pk_ts &&
		curr_ci_cj_pk_ts == curr_ci_pj_ck_ts &&
		curr_ci_pj_ck_ts == curr_pi_cj_ck_ts
	) {
		// okay, three neighbor curr cells have the same timestep
	}
	else {
		std::cerr << std::format(
			"\t\t\"curr\" neighbors are not equal, got:\n"
			"\t\tvolt({}, {}, {}) = {}\n"
			"\t\tcurr({}, {}, {}) = {}\n"
			"\t\tcurr({}, {}, {}) = {}\n"
			"\t\tcurr({}, {}, {}) = {}\n"
			"\t\tcurr({}, {}, {}) = {}\n",
			i, j, k, volt_ci_cj_ck_ts,
			i, j, k, curr_ci_cj_ck_ts,
			prev_i, j, k, curr_pi_cj_ck_ts,
			i, prev_j, k, curr_ci_pj_ck_ts,
			i, j, prev_k, curr_ci_cj_pk_ts
		);
		throw std::runtime_error(std::format(
			"checkVoltageKernel failed at ({}, {}, {}) ",
			i, j, k
		));
	}

	if (volt_ci_cj_ck_ts == curr_ci_cj_ck_ts + 1) {
		// okay, volt is one step ahead of curr
	}
	else {
		throw std::runtime_error(std::format(
			"checkVoltageKernel failed at ({}, {}, {}), "
			"expected {}, got {}!",
			i, j, k,
			curr_ci_cj_ck_ts + 1, volt_ci_cj_ck_ts
		));
	}
}

void checkCurrentKernel(
	Array3D<uint32_t>& curr,
	Array3D<uint32_t>& volt,
	uint32_t i, uint32_t j, uint32_t k
)
{
	size_t curr_ci_cj_ck_ts = curr(i, j, k);

	size_t volt_ci_cj_ck_ts = volt(i, j, k);
	size_t volt_ci_cj_nk_ts = volt(i, j, k+1);
	size_t volt_ci_nj_ck_ts = volt(i, j+1, k);
	size_t volt_ni_cj_ck_ts = volt(i+1, j, k);

	curr_ci_cj_ck_ts++;
	curr(i, j, k) = curr_ci_cj_ck_ts;

	if (volt_ci_cj_ck_ts == volt_ci_cj_nk_ts &&
		volt_ci_cj_nk_ts == volt_ci_nj_ck_ts &&
		volt_ci_nj_ck_ts == volt_ni_cj_ck_ts
	) {
		// okay, three neighbor volt cells have the same timestep
	}
	else {
		throw std::runtime_error(
			"checkCurrentKernel failed, \"volt\" neighbors are not equal."
		);
	}

	if (curr_ci_cj_ck_ts == volt_ci_cj_ck_ts) {
		// okay (due to leapfrog, curr is half a step behind volt)
	}
	else {
		throw std::runtime_error(std::format(
			"checkCurrentKernel failed, expected {}, got {}!",
			volt_ci_cj_ck_ts, curr_ci_cj_ck_ts
		));
	}
}

void checkVoltageRange(
	Array3D<uint32_t>& volt,
	Array3D<uint32_t>& curr,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
)
{
	if (debug) {
		std::cerr << std::format(
			"\tchecking volt({}, {}, {}) - volt({}, {}, {})\n",
			first[0], first[1], first[2],
			 last[0],  last[1],  last[2]
		);
	}

	for (uint32_t i = first[0]; i <= last[0]; i++) {
		for (uint32_t j = first[1]; j <= last[1]; j++) {
			for (uint32_t k = first[2]; k <= last[2]; k++) {
				checkVoltageKernel(volt, curr, i, j, k);
			}
		}
	}
}

void checkCurrentRange(
	Array3D<uint32_t>& curr,
	Array3D<uint32_t>& volt,
	std::array<size_t, 3> first,
	std::array<size_t, 3> last,
	bool debug
)
{
	if (debug) {
		std::cerr << std::format(
			"\tchecking curr({}, {}, {}) - curr({}, {}, {})\n",
			first[0], first[1], first[2],
			 last[0],  last[1],  last[2]
		);
	}

	for (uint32_t i = first[0]; i <= last[0]; i++) {
		for (uint32_t j = first[1]; j <= last[1]; j++) {
			for (uint32_t k = first[2]; k <= last[2]; k++) {
				checkCurrentKernel(curr, volt, i, j, k);
			}
		}
	}
}
