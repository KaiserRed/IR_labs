#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

class Tokenizer {
public:
    std::vector<std::string> tokenize(const std::string& text);
    std::string toLowerUtf8(const std::string& s);

private:
    bool isRussianLetter(unsigned char c1, unsigned char c2);
    bool isLetterByte(const std::string& s, size_t pos);
};

#endif
