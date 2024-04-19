#pragma once
#include <cstddef>
#include <stdexcept>
#include <algorithm>

template<typename T>
class Array3D
{
public:
	Array3D(size_t maxI, size_t maxJ, size_t maxK)
	{
		size_t size = maxI * maxJ * maxK;
		m_size = size;
		m_maxI = maxI;
		m_maxJ = maxJ;
		m_maxK = maxK;
		m_strideI = maxJ * maxK;
		m_strideJ = maxK;

		m_ptr = new T[size];
		std::fill(m_ptr, m_ptr + size, 0);
	}

	~Array3D()
	{
		delete[] m_ptr;
		m_ptr = NULL;
	}

	T& operator() (size_t i, size_t j, size_t k)
	{
		size_t idx = i * m_strideI + j * m_strideJ + k;
		if (i > m_maxI - 1 ||
			j > m_maxJ - 1 ||
			k > m_maxK - 1 ||
			idx > m_size - 1
		) {
			throw std::runtime_error("oob access");
		}

		return m_ptr[idx];
	}

	size_t i(void) { return m_maxI; }
	size_t j(void) { return m_maxJ; }
	size_t k(void) { return m_maxK; }

private:
	size_t m_size;
	size_t m_maxI, m_maxJ, m_maxK;
	size_t m_strideI, m_strideJ;
	T* m_ptr;
};
