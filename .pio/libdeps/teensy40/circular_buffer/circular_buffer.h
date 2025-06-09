#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER


template<class T>
class circular_buffer
{
public:
//    circular_buffer();
    circular_buffer(int capacity);
    ~circular_buffer();

    int get_capacity(); // get the maximum capacity of the buf
    int get_size();     // get the current item count

    void push_back(T item);
//    void push_front(T item);

    void pop_back();
    void pop_front();

    T& front();
    T& back();

    T& at(int index);
    T& operator[](int index);


protected:

    int capacity;
    int size;
    int start_pos;
    int end_pos;

    T *buffer;

    void increment(int& index);
    void decrement(int& index);

    int if_increment(int index);
    int if_decrement(int index);

    void reset();


};

#include "circular_buffer.cpp"

#endif // CIRCULAR_BUFFER


