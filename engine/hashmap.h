#ifndef HASHMAP_H
#define HASHMAP_H

#include <cstring>
#include <cstdlib>

template<typename V>
class HashMap {
public:
    struct Entry {
        char*  key;
        V      value;
        Entry* next;

        Entry(const char* k, const V& v) : next(0) {
            int len = strlen(k);
            key = new char[len + 1];
            memcpy(key, k, len + 1);
            value = v;
        }
        ~Entry() { delete[] key; }
    };

private:
    Entry** buckets_;
    int     nbuckets_;
    int     size_;

    unsigned hash(const char* k) const {
        unsigned h = 2166136261u;
        for (; *k; k++) {
            h ^= (unsigned char)*k;
            h *= 16777619u;
        }
        return h;
    }

    void rehash() {
        int nn = nbuckets_ * 2;
        Entry** nb = new Entry*[nn];
        memset(nb, 0, sizeof(Entry*) * nn);
        for (int i = 0; i < nbuckets_; i++) {
            Entry* e = buckets_[i];
            while (e) {
                Entry* nx = e->next;
                unsigned idx = hash(e->key) % nn;
                e->next = nb[idx];
                nb[idx] = e;
                e = nx;
            }
        }
        delete[] buckets_;
        buckets_ = nb;
        nbuckets_ = nn;
    }

    HashMap(const HashMap&);
    HashMap& operator=(const HashMap&);

public:
    HashMap(int initial = 4096) : nbuckets_(initial), size_(0) {
        buckets_ = new Entry*[nbuckets_];
        memset(buckets_, 0, sizeof(Entry*) * nbuckets_);
    }

    ~HashMap() {
        for (int i = 0; i < nbuckets_; i++) {
            Entry* e = buckets_[i];
            while (e) {
                Entry* nx = e->next;
                delete e;
                e = nx;
            }
        }
        delete[] buckets_;
    }

    V& operator[](const char* key) {
        unsigned idx = hash(key) % nbuckets_;
        Entry* e = buckets_[idx];
        while (e) {
            if (strcmp(e->key, key) == 0)
                return e->value;
            e = e->next;
        }
        if (size_ > nbuckets_ * 3 / 4) {
            rehash();
            idx = hash(key) % nbuckets_;
        }
        Entry* ne = new Entry(key, V());
        ne->next = buckets_[idx];
        buckets_[idx] = ne;
        size_++;
        return ne->value;
    }

    V* get(const char* key) {
        unsigned idx = hash(key) % nbuckets_;
        Entry* e = buckets_[idx];
        while (e) {
            if (strcmp(e->key, key) == 0)
                return &e->value;
            e = e->next;
        }
        return 0;
    }

    bool contains(const char* key) const {
        unsigned idx = hash(key) % nbuckets_;
        Entry* e = buckets_[idx];
        while (e) {
            if (strcmp(e->key, key) == 0)
                return true;
            e = e->next;
        }
        return false;
    }

    int len() const { return size_; }

    struct Iterator {
        Entry** buckets;
        int     nbuckets;
        int     bidx;
        Entry*  cur;

        void advance() {
            if (cur) cur = cur->next;
            while (!cur && bidx < nbuckets - 1) {
                bidx++;
                cur = buckets[bidx];
            }
        }

        bool valid() const { return cur != 0; }
        const char* key() const { return cur->key; }
        V& value() { return cur->value; }
        const V& value() const { return cur->value; }
    };

    Iterator iter() const {
        Iterator it;
        it.buckets = buckets_;
        it.nbuckets = nbuckets_;
        it.bidx = -1;
        it.cur = 0;
        for (int i = 0; i < nbuckets_; i++) {
            if (buckets_[i]) {
                it.bidx = i;
                it.cur = buckets_[i];
                break;
            }
        }
        return it;
    }
};

#endif
