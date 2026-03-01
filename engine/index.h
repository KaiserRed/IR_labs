#ifndef INDEX_H
#define INDEX_H

#include <cstring>
#include "dynarray.h"
#include "hashmap.h"

struct Posting {
    int    docId;
    int    count;
    char*  snippet;

    Posting() : docId(0), count(0), snippet(0) {}
    ~Posting() { delete[] snippet; }
    Posting(const Posting& o) : docId(o.docId), count(o.count), snippet(0) {
        if (o.snippet) { int n = strlen(o.snippet); snippet = new char[n+1]; memcpy(snippet, o.snippet, n+1); }
    }
    Posting& operator=(const Posting& o) {
        if (this != &o) {
            delete[] snippet; snippet = 0;
            docId = o.docId; count = o.count;
            if (o.snippet) { int n = strlen(o.snippet); snippet = new char[n+1]; memcpy(snippet, o.snippet, n+1); }
        }
        return *this;
    }
    bool operator>(const Posting& o) const { return docId > o.docId; }
    bool operator<(const Posting& o) const { return docId < o.docId; }
};

struct DocInfo {
    int   docId;
    char* title;
    char* url;

    DocInfo() : docId(0), title(0), url(0) {}
    ~DocInfo() { delete[] title; delete[] url; }
    DocInfo(const DocInfo& o) : docId(o.docId), title(0), url(0) {
        if (o.title) { int n = strlen(o.title); title = new char[n+1]; memcpy(title, o.title, n+1); }
        if (o.url)   { int n = strlen(o.url);   url   = new char[n+1]; memcpy(url,   o.url,   n+1); }
    }
    DocInfo& operator=(const DocInfo& o) {
        if (this != &o) {
            delete[] title; delete[] url;
            docId = o.docId; title = 0; url = 0;
            if (o.title) { int n = strlen(o.title); title = new char[n+1]; memcpy(title, o.title, n+1); }
            if (o.url)   { int n = strlen(o.url);   url   = new char[n+1]; memcpy(url,   o.url,   n+1); }
        }
        return *this;
    }
};

class InvertedIndex {
public:
    InvertedIndex();
    ~InvertedIndex();

    void addDocument(int docId, const char* title, const char* url);
    void addTerm(int docId, const char* term, int count, const char* snippet = 0);
    void finalize();

    DynArray<int> getDocIds(const char* term);
    const char* getSnippetForDoc(const char* term, int docId);
    const DynArray<DocInfo>& getDocuments() const { return documents_; }
    int getDocCount() const { return documents_.size(); }

    void save(const char* filename);
    void load(const char* filename);

    HashMap<DynArray<Posting>>& getIndex() { return index_; }

private:
    HashMap<DynArray<Posting>> index_;
    DynArray<DocInfo>      documents_;
};

#endif
