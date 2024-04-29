// BSD Zero Clause License
// 
// Copyright (C) 2024 Yifeng Li
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef TILING_HPP
#define TILING_HPP

#include <cstdint>
#include <array>
#include <vector>

namespace Tiling {
	using size_t = std::size_t;

	template <typename T>
	struct WrappedVector
	{
	public:
		using iterator = std::vector<T>::iterator;
		using const_iterator = std::vector<T>::const_iterator;
		iterator       begin()        { return m_vector.begin();    }
		iterator       end()          { return m_vector.end();      }
		const_iterator begin()  const { return m_vector.begin();    }
		const_iterator end()    const { return m_vector.end();      }
		const_iterator cbegin() const { return m_vector.cbegin();   }
		const_iterator cend()   const { return m_vector.cend();     }
		size_t         size()   const { return m_vector.size();     }

		void     reserve(size_t numelem)       { m_vector.reserve(numelem); }
		void     push_back(T elem)             { m_vector.push_back(elem);  }
		T&       operator[] (size_t idx)       { return m_vector[idx];      }
		const T& operator[] (size_t idx) const { return m_vector[idx];      }

	protected:
		std::vector<T> m_vector;
	};

	template <typename T>
	struct Range1D
	{
		T first, last;
	};

	struct Tile1D : WrappedVector<Range1D<size_t>>
	{
	public:
		Tile1D(size_t id) : m_id(id) {}
		size_t id() const { return m_id; }

	private:
		size_t m_id;
	};

	using TileList1D = std::vector<Tile1D>;
	using Plan1D = std::vector<TileList1D>;

	Plan1D
	computeParallelogramTiles(
		size_t totalWidth, size_t tileWidth,
		size_t halfTimesteps
	);

	Plan1D
	computeTrapezoidTiles(
		size_t totalWidth, size_t tileWidth,
		size_t halfTimesteps
	);

	template <typename T>
	struct Range3D
	{
		std::array<size_t, 3> first;
		std::array<size_t, 3> last;
	};

	struct Subtile3D : WrappedVector<Range3D<size_t>>
	{
	public:
		Subtile3D() {}
		Subtile3D(size_t id) : m_id(id) {}
		size_t id() { return m_id; }

		void push_back(Range3D<size_t> range)
		{
			m_vector.push_back(range);
			for (size_t i = 0; i < 3; i++) {
				first[i] = std::min(range.first[i], first[i]);
				 last[i] = std::max(range.last[i],   last[i]);
			}
		}

		std::array<size_t, 3> first = {SIZE_MAX, SIZE_MAX, SIZE_MAX};
		std::array<size_t, 3> last  = {0, 0, 0};

	private:
		size_t m_id;
	};

	struct Tile3D : WrappedVector<Subtile3D>
	{
	public:
		Tile3D(std::array<size_t, 3> id) : m_id(id) {}
		std::array<size_t, 3> id() const { return m_id; }

	private:
		std::array<size_t, 3> m_id;
	};

	using TileList3D = std::vector<Tile3D>;
	using Plan3D = std::vector<TileList3D>;

	Plan3D
	combineTilesTTT(const Plan1D& i, const Plan1D& j, const Plan1D& k);

	Plan3D
	combineTilesTTP(const Plan1D& i, const Plan1D& j, const Plan1D& k);

	Plan3D
	toLocalCoords(Plan3D plan);

	void visualizeTiles(
		const Plan1D& plan,
		size_t totalWidth, size_t tileWidth,
		size_t halfTimesteps
	);
}

// hack: make it header-only for quick tests without build systems
#ifdef TILING_HEADER_ONLY
#include "tiling.cpp"
#endif

#endif  // TILING_HPP
