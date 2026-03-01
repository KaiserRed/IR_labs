#ifndef STEMMER_H
#define STEMMER_H

#include <string>

class Stemmer {
public:
    std::string stem(const std::string& word);

private:
    int findRV(const char* w, int len);
    int findR1(const char* w, int len);
    int findR2(const char* w, int len);
    bool isVowel(unsigned char c1, unsigned char c2);
    bool endsWith(const char* w, int wlen, const char* suf, int slen);
    bool tryRemove(char* w, int* wlen, int region, const char** suffixes, bool needPrecede);
};

#endif
