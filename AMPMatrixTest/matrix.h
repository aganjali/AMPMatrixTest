#pragma once
#include  "Helper.h"
#include <vector>
#include <amp.h>
#include <math.h>

using namespace std;

using namespace concurrency;

template<class T>
class matrix
{

	T *data;
	int rows, cols;

public:
	matrix() :rows(0), cols(0){
		data = new T[0];
	}
	matrix(int rs, int cs) :rows(rs), cols(cs){
		data = new T[rs * cs];
	}
	~matrix(){
		if (data != 0){ delete[] data;		data = NULL;}
	}

	void fill_random(T minv, T maxv);
	void print();
	template<int tile_size >
	matrix<T>& multiply_tile_amp(matrix<T>* mb);
	matrix<T>& multiply_cpu(matrix<T>* mb) ;
	
	T& operator()(int i, int j);
	T operator()(int i, int j) const;
	T get_element(int i, int j) ;
	void set_element(int i, int j, T v);

	/*bool verify(matrix<double>& v);
	bool verify(matrix<float>& v);*/
	bool verify(matrix<T>* v_res);
};

template<class T>
bool matrix<T>::verify(matrix<T>* v)
{
	bool passed = true;

	for (int i = 0; i < rows*cols; ++i)
	{
		if (v->data[i] != data[i])
		{
			passed = false;
			break;
		}
	}

	return passed;
}
template<>
bool matrix<float>::verify(matrix<float>* v)
{
	bool passed = true;

	for (int i = 0; i < rows*cols; ++i)
	{
		if (abs(v->data[i] != data[i]) > 1e-4)
		{
			passed = false;
			break;
		}
	}

	return passed;
}
template<>
bool matrix<double>::verify(matrix<double>* v)
{
	bool passed = true;

	for (int i = 0; i < rows*cols; ++i)
	{
		if (abs(v->data[i] != data[i]) > 1e-4)
		{
			passed = false;
			break;
		}
	}

	return passed;
}
template<class T>
matrix<T>& matrix<T>::multiply_cpu(matrix<T>* mb) 
{
	ASSERT((cols != mb->rows), "matrix dimensions don't match")
	matrix<T> *mc= new matrix<T>(rows, mb->cols);

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < mb->cols; j++)
		{
			T sum = 0;
			for (int k = 0; k < cols; k++)
			{
				sum += data[i * cols + k] * mb->data[k * mb->cols + j];
			}
			mc->set_element(i,j, sum);
		}
	}
	return (*mc);
}

template<class T>
template<int tile_size >
matrix<T>& matrix<T>::multiply_tile_amp(matrix<T>* mb)
{
	
	//const int tile_size;
	ASSERT((cols != mb->rows), "matrix dimensions don't match")
	matrix<T> *mc = new matrix<T>(rows, mb->cols);
	
	concurrency::extent<2> e_a(rows, cols), e_b(mb->rows, mb->cols), e_c(rows, mb->cols);
	
	array_view<const T, 2> d_a(e_a, this->data);
	array_view<const T, 2> d_b(e_b, mb->data);
	array_view<T, 2> d_c(e_c, mc->data);
	d_c.discard_data();

	concurrency::extent<2> res_domain(e_c);
	
	parallel_for_each(res_domain.tile<tile_size, tile_size>().pad(), [=](tiled_index<tile_size, tile_size> tidx) restrict(amp)
	{
		index<2> localIdx = tidx.local;
		index<2> globalIdx = tidx.global;
		T val = 0;
		

		for (int i = 0; i < (d_a.extent[1]); i += tile_size)
		{
			tile_static T loc_A[tile_size][tile_size];
			tile_static T loc_B[tile_size][tile_size];

			if (globalIdx[0] < d_a.extent[0] && i + localIdx[1] < d_a.extent[1])
				loc_A[localIdx[0]][localIdx[1]] = d_a(globalIdx[0], i + localIdx[1]);
			else
				loc_A[localIdx[0]][localIdx[1]] = 0;

			if (globalIdx[1] < d_b.extent[1] && i + localIdx[0] < d_b.extent[0])
				loc_B[localIdx[0]][localIdx[1]] = d_b(i + localIdx[0], globalIdx[1]);
			else
				loc_B[localIdx[0]][localIdx[1]] = 0;

			tidx.barrier.wait_with_tile_static_memory_fence();

			for (int k = 0; k < tile_size; k++)
			{
				val += loc_A[localIdx[0]][k] * loc_B[k][localIdx[1]];
			}

			tidx.barrier.wait_with_tile_static_memory_fence();
		}
		if (globalIdx[0] < d_c.extent[0] && globalIdx[1] < d_c.extent[1])
		{
			d_c[tidx.global] = val;
		}
	});
	d_c.synchronize();
	
	return (*mc);
}

template<class T>
void matrix<T>::fill_random(T minv, T maxv)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			T v = minv + (T) rand() / ((T) RAND_MAX / (T) (maxv - minv));
			set_element(i, j, v);
		}
	}
}

template<class T>
void matrix<T>::print()
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
			cout << get_element(i, j) << "\t";
		cout << endl;
	}
}
template<class T>
T& matrix<T>::operator()(int i, int j)
{

	ASSERT(i >= rows || j >= cols || i<0 || j<0, "index out of bound")
	return data[i*cols + j];
}

template<class T>
T matrix<T>::operator()(int i, int j) const
{
	return get_element(i, j);
}

template<class T>
T matrix<T>::get_element(int i, int j)
{
	ASSERT(i >= rows || j >= cols || i<0 || j<0, "index out of bounds")
	return data[i * cols + j];
}

template<class T>
void matrix<T>::set_element(int i, int j, T v)
{
	ASSERT(i >= rows || j >= cols || i < 0 || j < 0, "index out of bounds")
	data[i * cols + j] = v;
}
