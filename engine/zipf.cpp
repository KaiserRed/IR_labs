#include "zipf.h"
#include <cstdio>
#include <cmath>

void ZipfAnalyzer::analyze(InvertedIndex* index) {
    sorted_.clear();

    HashMap<DynArray<Posting>>& idx = index->getIndex();
    HashMap<DynArray<Posting>>::Iterator it = idx.iter();

    while (it.valid()) {
        int tf = 0;
        DynArray<Posting>& postings = it.value();
        for (int i = 0; i < postings.size(); i++) {
            tf += postings[i].count;
        }
        TermFreq termFreq(it.key(), tf);
        sorted_.push(termFreq);
        it.advance();
    }

    for (int i = 1; i < sorted_.size(); i++) {
        TermFreq key = sorted_[i];
        int j = i - 1;
        while (j >= 0 && sorted_[j].freq < key.freq) {
            sorted_[j + 1] = sorted_[j];
            j--;
        }
        sorted_[j + 1] = key;
    }

    fprintf(stderr, "Zipf analysis: %d unique terms\n", sorted_.size());
}

void ZipfAnalyzer::exportCSV(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Cannot open %s for writing\n", filename);
        return;
    }

    fprintf(f, "rank,term,frequency,log_rank,log_frequency\n");

    for (int i = 0; i < sorted_.size(); i++) {
        int rank = i + 1;
        int freq = sorted_[i].freq > 0 ? sorted_[i].freq : 1;
        fprintf(f, "%d,%s,%d,%.4f,%.4f\n",
                rank,
                sorted_[i].term,
                sorted_[i].freq,
                log2f((float)rank),
                log2f((float)freq));
    }

    fclose(f);
    fprintf(stderr, "Zipf CSV exported to %s (%d terms)\n", filename, sorted_.size());
}
