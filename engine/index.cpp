#include "index.h"
#include <cstdio>
#include <cstring>

InvertedIndex::InvertedIndex() {}
InvertedIndex::~InvertedIndex() {}

void InvertedIndex::addDocument(int docId, const char* title, const char* url) {
    DocInfo info;
    info.docId = docId;
    if (title) {
        int n = strlen(title);
        info.title = new char[n + 1];
        memcpy(info.title, title, n + 1);
    }
    if (url) {
        int n = strlen(url);
        info.url = new char[n + 1];
        memcpy(info.url, url, n + 1);
    }
    documents_.push(info);
    info.title = 0;
    info.url = 0;
}

void InvertedIndex::addTerm(int docId, const char* term, int count, const char* snippet) {
    Posting p;
    p.docId = docId;
    p.count = count;
    if (snippet) {
        int n = strlen(snippet);
        p.snippet = new char[n + 1];
        memcpy(p.snippet, snippet, n + 1);
    }
    index_[term].push(p);
}

const char* InvertedIndex::getSnippetForDoc(const char* term, int docId) {
    DynArray<Posting>* postings = index_.get(term);
    if (!postings) return 0;
    for (int i = 0; i < postings->size(); i++) {
        if ((*postings)[i].docId == docId)
            return (*postings)[i].snippet;
    }
    return 0;
}

void InvertedIndex::finalize() {
    HashMap<DynArray<Posting>>::Iterator it = index_.iter();
    while (it.valid()) {
        it.value().sort();
        it.advance();
    }
    fprintf(stderr, "Index finalized: %d terms, %d documents\n", index_.len(), documents_.size());
}

DynArray<int> InvertedIndex::getDocIds(const char* term) {
    DynArray<int> result;
    DynArray<Posting>* postings = index_.get(term);
    if (!postings) return result;
    for (int i = 0; i < postings->size(); i++) {
        result.push((*postings)[i].docId);
    }
    return result;
}

static void writeInt(FILE* f, int v) {
    fwrite(&v, 4, 1, f);
}

static int readInt(FILE* f) {
    int v = 0;
    fread(&v, 4, 1, f);
    return v;
}

static void writeStr(FILE* f, const char* s) {
    int len = s ? strlen(s) : 0;
    writeInt(f, len);
    if (len > 0) fwrite(s, 1, len, f);
}

static char* readStr(FILE* f) {
    int len = readInt(f);
    if (len <= 0) return 0;
    char* s = new char[len + 1];
    fread(s, 1, len, f);
    s[len] = '\0';
    return s;
}

void InvertedIndex::save(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Cannot open %s for writing\n", filename);
        return;
    }

    fwrite("IRIDX003", 8, 1, f);

    writeInt(f, documents_.size());
    for (int i = 0; i < documents_.size(); i++) {
        writeInt(f, documents_[i].docId);
        writeStr(f, documents_[i].title);
        writeStr(f, documents_[i].url);
    }

    // Terms
    writeInt(f, index_.len());
    HashMap<DynArray<Posting>>::Iterator it = index_.iter();
    while (it.valid()) {
        writeStr(f, it.key());
        DynArray<Posting>& postings = it.value();
        writeInt(f, postings.size());
        for (int i = 0; i < postings.size(); i++) {
            writeInt(f, postings[i].docId);
            writeInt(f, postings[i].count);
            writeStr(f, postings[i].snippet);
        }
        it.advance();
    }

    fclose(f);
    fprintf(stderr, "Index saved to %s\n", filename);
}

void InvertedIndex::load(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s for reading\n", filename);
        return;
    }

    char magic[8];
    fread(magic, 8, 1, f);
    bool v3 = (memcmp(magic, "IRIDX003", 8) == 0);
    bool v2 = (memcmp(magic, "IRIDX002", 8) == 0);
    if (!v3 && !v2 && memcmp(magic, "IRIDX001", 8) != 0) {
        fprintf(stderr, "Invalid index file format\n");
        fclose(f);
        return;
    }

    int numDocs = readInt(f);
    for (int i = 0; i < numDocs; i++) {
        DocInfo info;
        info.docId = readInt(f);
        info.title = readStr(f);
        info.url = readStr(f);
        documents_.push(info);
        info.title = 0;
        info.url = 0;
    }

    int numTerms = readInt(f);
    for (int i = 0; i < numTerms; i++) {
        char* term = readStr(f);
        if (!term) continue;
        int numPostings = readInt(f);
        DynArray<Posting>& postings = index_[term];
        for (int j = 0; j < numPostings; j++) {
            Posting p;
            p.docId = readInt(f);
            p.count = (v2 || v3) ? readInt(f) : 1;
            p.snippet = v3 ? readStr(f) : 0;
            postings.push(p);
        }
        delete[] term;
    }

    fclose(f);
    fprintf(stderr, "Index loaded: %d terms, %d documents\n", index_.len(), documents_.size());
}
