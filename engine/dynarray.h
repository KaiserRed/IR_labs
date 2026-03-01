#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <cstdlib>
#include <cstring>

template<typename T>
class DynArray {
private:
    T*  data_;
    int size_;
    int cap_;

    void grow(int need) {
        if (need <= cap_) return;
        int nc = cap_ < 4 ? 4 : cap_;
        while (nc < need) nc *= 2;
        T* nd = new T[nc];
        for (int i = 0; i < size_; i++)
            nd[i] = data_[i];
        delete[] data_;
        data_ = nd;
        cap_ = nc;
    }

public:
    DynArray() : data_(0), size_(0), cap_(0) {}

    ~DynArray() { delete[] data_; }

    DynArray(const DynArray& o) : data_(0), size_(0), cap_(0) {
        if (o.size_ > 0) {
            grow(o.size_);
            for (int i = 0; i < o.size_; i++)
                data_[i] = o.data_[i];
            size_ = o.size_;
        }
    }

    DynArray& operator=(const DynArray& o) {
        if (this != &o) {
            delete[] data_;
            data_ = 0;
            size_ = 0;
            cap_ = 0;
            if (o.size_ > 0) {
                grow(o.size_);
                for (int i = 0; i < o.size_; i++)
                    data_[i] = o.data_[i];
                size_ = o.size_;
            }
        }
        return *this;
    }

    void push(const T& val) {
        grow(size_ + 1);
        data_[size_++] = val;
    }

    T& operator[](int i) { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

    void clear() { size_ = 0; }

    void sort() {
        for (int i = 1; i < size_; i++) {
            T key = data_[i];
            int j = i - 1;
            while (j >= 0 && data_[j] > key) {
                data_[j + 1] = data_[j];
                j--;
            }
            data_[j + 1] = key;
        }
    }

    void sortDescByValue() {
        for (int i = 1; i < size_; i++) {
            T key = data_[i];
            int j = i - 1;
            while (j >= 0 && data_[j] < key) {
                data_[j + 1] = data_[j];
                j--;
            }
            data_[j + 1] = key;
        }
    }

    void removeDuplicatesSorted() {
        if (size_ < 2) return;
        int w = 1;
        for (int r = 1; r < size_; r++) {
            if (!(data_[r] == data_[w - 1]))
                data_[w++] = data_[r];
        }
        size_ = w;
    }
};

#endif
