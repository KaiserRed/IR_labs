#ifndef SEARCH_H
#define SEARCH_H

#include <string>
#include "index.h"
#include "stemmer.h"
#include "tokenizer.h"

class BooleanSearch {
public:
    BooleanSearch(InvertedIndex* idx);

    DynArray<int> search(const char* query);
    std::string getFirstTerm(const char* query);

private:
    InvertedIndex* index_;
    Tokenizer tokenizer_;
    Stemmer stemmer_;

    const char* pos_;
    DynArray<int> parseOrExpr();
    DynArray<int> parseAndExpr();
    DynArray<int> parseNotExpr();
    DynArray<int> parseAtom();

    void skipSpaces();
    bool matchKeyword(const char* kw);

    static DynArray<int> intersect(const DynArray<int>& a, const DynArray<int>& b);
    static DynArray<int> unite(const DynArray<int>& a, const DynArray<int>& b);
    DynArray<int> complement(const DynArray<int>& a);

    DynArray<int> allDocs();
};

#endif
