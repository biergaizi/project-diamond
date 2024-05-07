// Quick and dirty 4D array implementation to represent a 3D vector
// field inside a 3D space.
//
// The entire space is represented as (maxI, maxJ, maxK), and at each
// cell of this space there exists a 3D vector (x, y, z). Thus, every
// element is addressed by a 4D coordinate (x, y, z, n).
//
// This array must always be passed via reference, not value, because
// it's a memory wrapper and different copies hold the same underlying
// pointer. When RAII frees a single array within a scope, it affects
// the entire program.

#pragma once
#include <cstddef>
#include <stdexcept>
#include <algorithm>

template<typename T, size_t maxN=3>
class NArray3D
{
public:
	NArray3D(std::string name, std::array<size_t, 3> size)
	{
		m_name = name;

		m_elems = size[0] * size[1] * size[2] * maxN;
		m_size = size;

		m_strideI = size[1] * size[2] * maxN;
		m_strideJ = size[2] * maxN;
		m_strideK = maxN;

		m_ptr = new T[m_elems];
		std::fill(m_ptr, m_ptr + m_elems, 0);
	}

	~NArray3D()
	{
		delete[] m_ptr;
		m_ptr = NULL;
	}

	T& operator() (size_t i, size_t j, size_t k, size_t n) const
	{
		size_t idx = i * m_strideI + j * m_strideJ + k * m_strideK + n;
		if (i > m_size[0] - 1 ||
			j > m_size[1] - 1 ||
			k > m_size[2] - 1 ||
			n > maxN - 1   ||
			idx > m_elems - 1
		) {
			throw std::runtime_error("oob access");
		}

		return m_ptr[idx];
	}

	size_t i() const { return m_size[0]; }
	size_t j() const { return m_size[1]; }
	size_t k() const { return m_size[2]; }
	std::string name() { return m_name; };

private:
	std::array<size_t, 3> m_size;
	std::string m_name;
	size_t m_elems;
	size_t m_strideI, m_strideJ, m_strideK;
	T* m_ptr;
};
