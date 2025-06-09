
#ifndef CIRCULAR_BUFFER_CPP
#define CIRCULAR_BUFFER_CPP

#include <stdio.h>
#include "circular_buffer.h"

/*
template<class T>
circular_buffer<T>::circular_buffer()
{
    capacity = 0;
    size = 0;

    buffer = 0;

    reset();
}
*/

template<class T>
circular_buffer<T>::circular_buffer(int tcapacity)
{
    capacity = tcapacity;
    buffer = new T[capacity];

    reset();
}

template<class T>
circular_buffer<T>::~circular_buffer()
{
    if (buffer != 0)
        delete[] buffer;
}

template<class T>
void circular_buffer<T>::reset()
{
    start_pos = 0;
    end_pos = 0;
    size = 0;
}

template<class T>
int circular_buffer<T>::get_capacity()
{
    return capacity;
}

template<class T>
int circular_buffer<T>::get_size()
{
    return size;
}

template<class T>
void circular_buffer<T>::push_back(T item)
{
    if (size  == capacity)
        pop_front();

    buffer[end_pos] = item;
    increment(end_pos);
    size++;
}

/*
template<class T>
void circular_buffer<T>::push_front(T item)
{
    if (size  == capacity)
        pop_back();

    buffer[start_pos] = item;
    decrement(start_pos);
    size++;
}

*/

template<class T>
void circular_buffer<T>::pop_back()
{
    if (size != 0)
    {
        size--;
        decrement(end_pos);
    }
}

template<class T>
void circular_buffer<T>::pop_front()
{
    if (size != 0)
    {
        size--;
        increment(start_pos);
    }
}

template<class T>
void circular_buffer<T>::increment(int& index)
{
    index++;
    if (index >= capacity)
        index = 0;
}

template<class T>
void circular_buffer<T>::decrement(int& index)
{
    index--;
    if (index < 0)
        index = capacity - 1;
}


template<class T>
int circular_buffer<T>::if_increment(int index)
{
    index++;
    if (index >= capacity)
        index = 0;

    return index;
}

template<class T>
int circular_buffer<T>::if_decrement(int index)
{
    index--;
    if (index < capacity)
        index = capacity - 1;

    return index;
}

template<class T>
T& circular_buffer<T>::front()
{
    return buffer[start_pos];
}

template<class T>
T& circular_buffer<T>::back()
{
    return buffer[if_decrement(end_pos)];
}


template<class T>
T& circular_buffer<T>::operator[](int index)
{
    int real_index = 0;

//    if (size == 0)  // no item
//        return NULL;

    real_index = index + start_pos;
    if (real_index >= capacity)
        real_index -= capacity;

    return buffer[real_index];
}


template<class T>
T& circular_buffer<T>::at(int index)
{
    int real_index = 0;

    real_index = index + start_pos;
    if (real_index >= capacity)
        real_index -= capacity;

    return buffer[real_index];
}

#endif // CIRCULAR_BUFFER_CPP
