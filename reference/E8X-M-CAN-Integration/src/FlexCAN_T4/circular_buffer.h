#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

#include <stdint.h>

template<typename T, uint16_t _size>
class circular_buffer {
public:
    circular_buffer() : head(0), tail(0), count(0) {}

    bool push(const T &item) {
        if (count >= _size) return false;
        buffer[head] = item;
        head = (head + 1) % _size;
        count++;
        return true;
    }

    bool pop(T &item) {
        if (count == 0) return false;
        item = buffer[tail];
        tail = (tail + 1) % _size;
        count--;
        return true;
    }

    bool peek(T &item) const {
        if (count == 0) return false;
        item = buffer[tail];
        return true;
    }

    void clear() {
        head = tail = count = 0;
    }

    bool isEmpty() const {
        return count == 0;
    }

    bool isFull() const {
        return count == _size;
    }

    uint16_t size() const {
        return count;
    }

private:
    T buffer[_size];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
};

#endif 