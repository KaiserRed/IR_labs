#include "search.h"
#include <cstring>
#include <cstdio>
#include <cctype>

BooleanSearch::BooleanSearch(InvertedIndex* idx) : index_(idx), pos_(0) {}

void BooleanSearch::skipSpaces() {
    while (*pos_ == ' ' || *pos_ == '\t') pos_++;
}

bool BooleanSearch::matchKeyword(const char* kw) {
    skipSpaces();
    int len = strlen(kw);
    if (strncmp(pos_, kw, len) == 0 && (pos_[len] == ' ' || pos_[len] == '(' || pos_[len] == '\0')) {
        pos_ += len;
        return true;
    }
    return false;
}

DynArray<int> BooleanSearch::search(const char* query) {
    pos_ = query;
    return parseOrExpr();
}

std::string BooleanSearch::getFirstTerm(const char* query) {
    pos_ = query;
    skipSpaces();
    while (*pos_) {
        if (*pos_ == '(') { pos_++; skipSpaces(); continue; }
        if (matchKeyword("AND") || matchKeyword("OR") || matchKeyword("NOT")) continue;
        const char* start = pos_;
        while (*pos_ && *pos_ != ' ' && *pos_ != ')' && *pos_ != '(' && *pos_ != '\t') {
            unsigned char c = *pos_;
            if (c >= 0xC0 && c < 0xE0) pos_ += 2;
            else if (c >= 0xE0 && c < 0xF0) pos_ += 3;
            else pos_ += 1;
        }
        if (pos_ > start) {
            std::string s(start, pos_ - start);
            std::vector<std::string> tokens = tokenizer_.tokenize(s);
            if (!tokens.empty())
                return stemmer_.stem(tokens[0]);
        }
        break;
    }
    return "";
}

DynArray<int> BooleanSearch::parseOrExpr() {
    DynArray<int> result = parseAndExpr();

    while (true) {
        skipSpaces();
        if (matchKeyword("OR") || matchKeyword("or") || matchKeyword("ИЛИ")) {
            DynArray<int> right = parseAndExpr();
            result = unite(result, right);
        } else {
            break;
        }
    }
    return result;
}

DynArray<int> BooleanSearch::parseAndExpr() {
    DynArray<int> result = parseNotExpr();

    while (true) {
        skipSpaces();
        if (matchKeyword("AND") || matchKeyword("and") || matchKeyword("И")) {
            DynArray<int> right = parseNotExpr();
            result = intersect(result, right);
        } else {
            break;
        }
    }
    return result;
}

DynArray<int> BooleanSearch::parseNotExpr() {
    skipSpaces();
    if (matchKeyword("NOT") || matchKeyword("not") || matchKeyword("НЕ")) {
        DynArray<int> operand = parseAtom();
        return complement(operand);
    }
    return parseAtom();
}

DynArray<int> BooleanSearch::parseAtom() {
    skipSpaces();

    if (*pos_ == '(') {
        pos_++;
        DynArray<int> result = parseOrExpr();
        skipSpaces();
        if (*pos_ == ')') pos_++;
        return result;
    }

    const char* start = pos_;
    while (*pos_ && *pos_ != ' ' && *pos_ != ')' && *pos_ != '(' && *pos_ != '\t') {
        unsigned char c = *pos_;
        if (c >= 0xC0 && c < 0xE0) {
            pos_ += 2;
        } else if (c >= 0xE0 && c < 0xF0) {
            pos_ += 3;
        } else {
            pos_ += 1;
        }
    }

    int termLen = pos_ - start;
    if (termLen == 0) {
        return DynArray<int>();
    }

    char* rawTerm = new char[termLen + 1];
    memcpy(rawTerm, start, termLen);
    rawTerm[termLen] = '\0';

    // Tokenize and stem
    std::vector<std::string> tokens = tokenizer_.tokenize(std::string(rawTerm));
    delete[] rawTerm;

    if (tokens.empty()) {
        return DynArray<int>();
    }

    std::string stemmed = stemmer_.stem(tokens[0]);
    return index_->getDocIds(stemmed.c_str());
}

DynArray<int> BooleanSearch::intersect(const DynArray<int>& a, const DynArray<int>& b) {
    DynArray<int> result;
    int i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push(a[i]);
            i++; j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }
    return result;
}

DynArray<int> BooleanSearch::unite(const DynArray<int>& a, const DynArray<int>& b) {
    DynArray<int> result;
    int i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push(a[i]);
            i++; j++;
        } else if (a[i] < b[j]) {
            result.push(a[i]);
            i++;
        } else {
            result.push(b[j]);
            j++;
        }
    }
    while (i < a.size()) result.push(a[i++]);
    while (j < b.size()) result.push(b[j++]);
    return result;
}

DynArray<int> BooleanSearch::complement(const DynArray<int>& a) {
    DynArray<int> all = allDocs();
    DynArray<int> result;
    int i = 0, j = 0;
    while (i < all.size()) {
        if (j < a.size() && all[i] == a[j]) {
            i++; j++;
        } else {
            result.push(all[i]);
            i++;
        }
    }
    return result;
}

DynArray<int> BooleanSearch::allDocs() {
    DynArray<int> result;
    const DynArray<DocInfo>& docs = index_->getDocuments();
    for (int i = 0; i < docs.size(); i++) {
        result.push(docs[i].docId);
    }
    result.sort();
    return result;
}
