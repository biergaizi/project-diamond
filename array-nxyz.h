#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

template <typename T>
struct N_3DArray
{
	inline T& operator() (const unsigned int n, const unsigned int x, const unsigned int y, const unsigned int z)
	{
		return array.at(
		           n * n_stride +
		           x * x_stride +
		           y * y_stride +
		           z
		       );
	}

	const inline T operator() (const unsigned int &n, const unsigned int &x, const unsigned int &y, const unsigned int &z) const
	{
		return array.at(
		           n * n_stride +
		           x * x_stride +
		           y * y_stride +
		           z
		       );
	}

	unsigned long n_stride, x_stride, y_stride;

	std::vector<T> array;
};

template <typename T>
N_3DArray<T>* Create_N_3DArray(const unsigned int* numLines)
{
	//unsigned int n_max = 3;
	unsigned int x_max = numLines[0];
	unsigned int y_max = numLines[1];
	unsigned int z_max = numLines[2];

	unsigned long n_stride = x_max * y_max * z_max;
	unsigned long x_stride = y_max * z_max;
	unsigned long y_stride = z_max;

	N_3DArray<T>* array = new N_3DArray<T>;
	array->n_stride = n_stride;
	array->x_stride = x_stride;
	array->y_stride = y_stride;

	return array;
}
