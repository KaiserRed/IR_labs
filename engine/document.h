#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "dynarray.h"

struct RawDocument {
    int   page_id;
    char* title;
    char* url;
    char* html;

    RawDocument() : page_id(0), title(0), url(0), html(0) {}
    ~RawDocument() { delete[] title; delete[] url; delete[] html; }

    RawDocument(const RawDocument& o) : page_id(o.page_id), title(0), url(0), html(0) {
        if (o.title) { int n = strlen(o.title); title = new char[n+1]; memcpy(title, o.title, n+1); }
        if (o.url)   { int n = strlen(o.url);   url   = new char[n+1]; memcpy(url,   o.url,   n+1); }
        if (o.html)  { int n = strlen(o.html);  html  = new char[n+1]; memcpy(html,  o.html,  n+1); }
    }

    RawDocument& operator=(const RawDocument& o) {
        if (this != &o) {
            delete[] title; delete[] url; delete[] html;
            page_id = o.page_id; title = 0; url = 0; html = 0;
            if (o.title) { int n = strlen(o.title); title = new char[n+1]; memcpy(title, o.title, n+1); }
            if (o.url)   { int n = strlen(o.url);   url   = new char[n+1]; memcpy(url,   o.url,   n+1); }
            if (o.html)  { int n = strlen(o.html);  html  = new char[n+1]; memcpy(html,  o.html,  n+1); }
        }
        return *this;
    }
};

DynArray<RawDocument> loadDocuments(const char* jsonlPath);

char* stripHTML(const char* html);

#endif
