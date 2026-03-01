#ifndef ZIPF_H
#define ZIPF_H

#include "index.h"

struct TermFreq {
    const char* term;
    int         freq;

    TermFreq() : term(0), freq(0) {}
    TermFreq(const char* t, int f) : term(t), freq(f) {}

    bool operator>(const TermFreq& o) const { return freq > o.freq; }
    bool operator<(const TermFreq& o) const { return freq < o.freq; }
};

class ZipfAnalyzer {
public:
    void analyze(InvertedIndex* index);
    void exportCSV(const char* filename);

private:
    DynArray<TermFreq> sorted_;
};

#endif
