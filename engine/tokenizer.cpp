#include "tokenizer.h"
#include <cctype>

bool Tokenizer::isRussianLetter(unsigned char c1, unsigned char c2) {
    // Uppercase А-П: 0xD0 0x90-0x9F
    // Uppercase Р-Я: 0xD0 0xA0-0xAF
    // Ё:             0xD0 0x81
    // Lowercase а-п: 0xD0 0xB0-0xBF
    // Lowercase р-я: 0xD1 0x80-0x8F
    // ё:             0xD1 0x91
    if (c1 == 0xD0) {
        return c2 == 0x81 ||
               (c2 >= 0x90 && c2 <= 0xBF);
    }
    if (c1 == 0xD1) {
        return (c2 >= 0x80 && c2 <= 0x8F) ||
               c2 == 0x91;
    }
    return false;
}

bool Tokenizer::isLetterByte(const std::string& s, size_t pos) {
    unsigned char c = s[pos];
    if (c < 0x80) return std::isalpha(c) != 0;
    if (pos + 1 < s.size()) {
        return isRussianLetter(c, (unsigned char)s[pos + 1]);
    }
    return false;
}

std::string Tokenizer::toLowerUtf8(const std::string& s) {
    std::string result;
    result.reserve(s.size());

    for (size_t i = 0; i < s.size(); ) {
        unsigned char c1 = s[i];

        if (c1 < 0x80) {
            result += (char)std::tolower(c1);
            i++;
        } else if (c1 == 0xD0 && i + 1 < s.size()) {
            unsigned char c2 = s[i + 1];
            if (c2 == 0x81) {
                // Ё -> ё
                result += (char)0xD1;
                result += (char)0x91;
            } else if (c2 >= 0x90 && c2 <= 0x9F) {
                // А-П -> а-п
                result += (char)0xD0;
                result += (char)(c2 + 0x20);
            } else if (c2 >= 0xA0 && c2 <= 0xAF) {
                // Р-Я -> р-я
                result += (char)0xD1;
                result += (char)(c2 - 0x20);
            } else {
                result += (char)c1;
                result += (char)c2;
            }
            i += 2;
        } else if (c1 >= 0xC0 && c1 < 0xE0 && i + 1 < s.size()) {
            result += (char)c1;
            result += s[i + 1];
            i += 2;
        } else if (c1 >= 0xE0 && c1 < 0xF0 && i + 2 < s.size()) {
            result += s.substr(i, 3);
            i += 3;
        } else {
            result += (char)c1;
            i++;
        }
    }
    return result;
}

std::vector<std::string> Tokenizer::tokenize(const std::string& text) {
    std::string lower = toLowerUtf8(text);
    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < lower.size(); ) {
        unsigned char c = lower[i];

        bool isLetter = false;
        int charBytes = 1;

        if (c < 0x80) {
            isLetter = std::isalpha(c) != 0;
            charBytes = 1;
        } else if (c >= 0xC0 && c < 0xE0 && i + 1 < lower.size()) {
            isLetter = isRussianLetter(c, (unsigned char)lower[i + 1]);
            charBytes = 2;
        } else if (c >= 0xE0 && c < 0xF0) {
            charBytes = 3;
        } else if (c >= 0xF0) {
            charBytes = 4;
        }

        if (isLetter) {
            current.append(lower, i, charBytes);
        } else {
            if (!current.empty() && current.size() >= 2) {
                tokens.push_back(current);
            }
            current.clear();
        }

        i += charBytes;
    }

    if (!current.empty() && current.size() >= 2) {
        tokens.push_back(current);
    }

    return tokens;
}
