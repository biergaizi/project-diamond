// Emulated SIMD type using for loop.
#pragma once
#include <cstddef>

template <typename Tscalar, size_t num>
struct Simd
{
	Tscalar elem[num];

	void operator= (const int val)
	{
		for (size_t i = 0; i < num; i++) {
			elem[i] = val;
		}
	}
};

template <typename Tscalar, size_t num>
bool
operator== (const Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b)
{
	for (size_t i = 0; i < num; i++) {
		if (a.elem[i] != b.elem[i]) {
			return false;
		}
	}
	return true;
}

template <typename Tscalar, size_t num>
Simd<Tscalar, num>
operator+ (const Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b)
{
	Simd<Tscalar, num> retval;

	for (size_t i = 0; i < num; i++) {
		retval.elem[i] = a.elem[i] + b.elem[i];
	}
	return retval;
}

template <typename Tscalar, size_t num>
Simd<Tscalar, num>
operator- (const Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b)
{
	Simd<Tscalar, num> retval;

	for (size_t i = 0; i < num; i++) {
		retval.elem[i] = a.elem[i] - b.elem[i];
	}
	return retval;
}

template <typename Tscalar, size_t num>
Simd<Tscalar, num>
operator* (const Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b)
{
	Simd<Tscalar, num> retval;

	for (size_t i = 0; i < num; i++) {
		retval.elem[i] = a.elem[i] * b.elem[i];
	}
	return retval;
}

template <typename Tscalar, size_t num>
void
operator+= (Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b) { a = a + b; }

template <typename Tscalar, size_t num>
void
operator*= (Simd<Tscalar, num>& a, const Simd<Tscalar, num>& b) { a = a * b; }
