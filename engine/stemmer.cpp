#include "stemmer.h"
#include <cstring>
#include <cstdlib>

bool Stemmer::isVowel(unsigned char c1, unsigned char c2) {
    if (c1 == 0xD0) {
        // а=B0 е=B5 и=B8 о=BE
        return c2 == 0xB0 || c2 == 0xB5 || c2 == 0xB8 || c2 == 0xBE;
    }
    if (c1 == 0xD1) {
        // у=83 ы=8B э=8D ю=8E я=8F ё=91
        return c2 == 0x83 || c2 == 0x8B || c2 == 0x8D ||
               c2 == 0x8E || c2 == 0x8F || c2 == 0x91;
    }
    return false;
}

int Stemmer::findRV(const char* w, int len) {
    for (int i = 0; i + 1 < len; ) {
        unsigned char c1 = w[i];
        if (c1 >= 0xC0 && c1 < 0xE0) {
            if (isVowel(c1, (unsigned char)w[i + 1]))
                return i + 2;
            i += 2;
        } else if (c1 < 0x80) {
            i += 1;
        } else {
            i += 1;
        }
    }
    return len;
}

int Stemmer::findR1(const char* w, int len) {
    bool seenVowel = false;
    for (int i = 0; i + 1 < len; ) {
        unsigned char c1 = w[i];
        int cb = (c1 >= 0xC0 && c1 < 0xE0) ? 2 : 1;
        if (cb == 2 && i + 1 < len) {
            bool v = isVowel(c1, (unsigned char)w[i + 1]);
            if (seenVowel && !v)
                return i + cb;
            if (v) seenVowel = true;
        }
        i += cb;
    }
    return len;
}

int Stemmer::findR2(const char* w, int len) {
    int r1 = findR1(w, len);
    if (r1 >= len) return len;
    bool seenVowel = false;
    for (int i = r1; i + 1 < len; ) {
        unsigned char c1 = w[i];
        int cb = (c1 >= 0xC0 && c1 < 0xE0) ? 2 : 1;
        if (cb == 2 && i + 1 < len) {
            bool v = isVowel(c1, (unsigned char)w[i + 1]);
            if (seenVowel && !v)
                return i + cb;
            if (v) seenVowel = true;
        }
        i += cb;
    }
    return len;
}

bool Stemmer::endsWith(const char* w, int wlen, const char* suf, int slen) {
    if (wlen < slen) return false;
    return memcmp(w + wlen - slen, suf, slen) == 0;
}

static bool precededByAorYa(const char* w, int wlen, int slen) {
    int pos = wlen - slen;
    if (pos < 2) return false;
    unsigned char c1 = w[pos - 2];
    unsigned char c2 = w[pos - 1];
    // а: D0 B0, я: D1 8F
    return (c1 == 0xD0 && c2 == 0xB0) || (c1 == 0xD1 && c2 == 0x8F);
}

bool Stemmer::tryRemove(char* w, int* wlen, int region, const char** suffixes, bool needPrecede) {
    for (int i = 0; suffixes[i]; i++) {
        int slen = strlen(suffixes[i]);
        if (*wlen - slen < region) continue;
        if (!endsWith(w, *wlen, suffixes[i], slen)) continue;
        if (needPrecede && !precededByAorYa(w, *wlen, slen)) continue;
        *wlen -= slen;
        w[*wlen] = '\0';
        return true;
    }
    return false;
}

// Suffix lists
static const char* perf_ger_g1[] = {
    "\xd0\xb2\xd1\x88\xd0\xb8\xd1\x81\xd1\x8c", // вшись
    "\xd0\xb2\xd1\x88\xd0\xb8",                   // вши
    "\xd0\xb2",                                     // в
    0
};
static const char* perf_ger_g2[] = {
    "\xd0\xb8\xd0\xb2\xd1\x88\xd0\xb8\xd1\x81\xd1\x8c", // ившись
    "\xd1\x8b\xd0\xb2\xd1\x88\xd0\xb8\xd1\x81\xd1\x8c", // ывшись
    "\xd0\xb8\xd0\xb2\xd1\x88\xd0\xb8",                   // ивши
    "\xd1\x8b\xd0\xb2\xd1\x88\xd0\xb8",                   // ывши
    "\xd0\xb8\xd0\xb2",                                     // ив
    "\xd1\x8b\xd0\xb2",                                     // ыв
    0
};

static const char* adj_suf[] = {
    "\xd0\xb8\xd0\xbc\xd0\xb8", // ими
    "\xd1\x8b\xd0\xbc\xd0\xb8", // ыми
    "\xd0\xb5\xd0\xb3\xd0\xbe", // его
    "\xd0\xbe\xd0\xb3\xd0\xbe", // ого
    "\xd0\xb5\xd0\xbc\xd1\x83", // ему
    "\xd0\xbe\xd0\xbc\xd1\x83", // ому
    "\xd0\xb5\xd0\xb5", // ее
    "\xd0\xb8\xd0\xb5", // ие
    "\xd1\x8b\xd0\xb5", // ые
    "\xd0\xbe\xd0\xb5", // ое
    "\xd0\xb5\xd0\xb9", // ей
    "\xd0\xb8\xd0\xb9", // ий
    "\xd1\x8b\xd0\xb9", // ый
    "\xd0\xbe\xd0\xb9", // ой
    "\xd0\xb5\xd0\xbc", // ем
    "\xd0\xb8\xd0\xbc", // им
    "\xd1\x8b\xd0\xbc", // ым
    "\xd0\xbe\xd0\xbc", // ом
    "\xd0\xb8\xd1\x85", // их
    "\xd1\x8b\xd1\x85", // ых
    "\xd1\x83\xd1\x8e", // ую
    "\xd1\x8e\xd1\x8e", // юю
    "\xd0\xb0\xd1\x8f", // ая
    "\xd1\x8f\xd1\x8f", // яя
    "\xd0\xbe\xd1\x8e", // ою
    "\xd0\xb5\xd1\x8e", // ею
    0
};

static const char* part_g1[] = {
    "\xd0\xb5\xd0\xbc", // ем
    "\xd0\xbd\xd0\xbd", // нн
    "\xd0\xb2\xd1\x88", // вш
    "\xd1\x8e\xd1\x89", // ющ
    "\xd1\x89",         // щ
    0
};
static const char* part_g2[] = {
    "\xd0\xb8\xd0\xb2\xd1\x88", // ивш
    "\xd1\x8b\xd0\xb2\xd1\x88", // ывш
    "\xd1\x83\xd1\x8e\xd1\x89", // ующ
    0
};

static const char* refl_suf[] = {
    "\xd1\x81\xd1\x8f", // ся
    "\xd1\x81\xd1\x8c", // сь
    0
};

static const char* verb_g1[] = {
    "\xd0\xb5\xd1\x88\xd1\x8c",             // ешь
    "\xd0\xbd\xd0\xbd\xd0\xbe",             // нно
    "\xd0\xb5\xd1\x82\xd0\xb5",             // ете
    "\xd0\xb9\xd1\x82\xd0\xb5",             // йте
    "\xd0\xbb\xd0\xb0",                     // ла
    "\xd0\xbd\xd0\xb0",                     // на
    "\xd0\xbb\xd0\xb8",                     // ли
    "\xd0\xb5\xd0\xbc",                     // ем
    "\xd0\xbb\xd0\xbe",                     // ло
    "\xd0\xbd\xd0\xbe",                     // но
    "\xd0\xb5\xd1\x82",                     // ет
    "\xd1\x8e\xd1\x82",                     // ют
    "\xd0\xbd\xd1\x8b",                     // ны
    "\xd1\x82\xd1\x8c",                     // ть
    "\xd0\xb9",                             // й
    "\xd0\xbb",                             // л
    "\xd0\xbd",                             // н
    0
};
static const char* verb_g2[] = {
    "\xd0\xb5\xd0\xb9\xd1\x82\xd0\xb5",     // ейте
    "\xd1\x83\xd0\xb9\xd1\x82\xd0\xb5",     // уйте
    "\xd0\xb8\xd0\xbb\xd0\xb0",             // ила
    "\xd1\x8b\xd0\xbb\xd0\xb0",             // ыла
    "\xd0\xb5\xd0\xbd\xd0\xb0",             // ена
    "\xd0\xb8\xd1\x82\xd0\xb5",             // ите
    "\xd0\xb8\xd0\xbb\xd0\xb8",             // или
    "\xd1\x8b\xd0\xbb\xd0\xb8",             // ыли
    "\xd0\xb8\xd0\xbb\xd0\xbe",             // ило
    "\xd1\x8b\xd0\xbb\xd0\xbe",             // ыло
    "\xd0\xb5\xd0\xbd\xd0\xbe",             // ено
    "\xd1\x83\xd0\xb5\xd1\x82",             // ует
    "\xd1\x83\xd1\x8e\xd1\x82",             // уют
    "\xd0\xb5\xd0\xbd\xd1\x8b",             // ены
    "\xd0\xb8\xd1\x82\xd1\x8c",             // ить
    "\xd1\x8b\xd1\x82\xd1\x8c",             // ыть
    "\xd0\xb8\xd1\x88\xd1\x8c",             // ишь
    "\xd0\xb5\xd0\xb9",                     // ей
    "\xd1\x83\xd0\xb9",                     // уй
    "\xd0\xb8\xd0\xbb",                     // ил
    "\xd1\x8b\xd0\xbb",                     // ыл
    "\xd0\xb8\xd0\xbc",                     // им
    "\xd1\x8b\xd0\xbc",                     // ым
    "\xd0\xb5\xd0\xbd",                     // ен
    "\xd1\x8f\xd1\x82",                     // ят
    "\xd0\xb8\xd1\x82",                     // ит
    "\xd1\x8b\xd1\x82",                     // ыт
    "\xd1\x83\xd1\x8e",                     // ую
    "\xd1\x8e",                             // ю
    0
};

static const char* noun_suf[] = {
    "\xd0\xb8\xd1\x8f\xd0\xbc\xd0\xb8",     // иями
    "\xd1\x8f\xd0\xbc\xd0\xb8",             // ями
    "\xd0\xb0\xd0\xbc\xd0\xb8",             // ами
    "\xd0\xb8\xd0\xb5\xd0\xb9",             // ией
    "\xd0\xb8\xd1\x8f\xd0\xbc",             // иям
    "\xd0\xb8\xd0\xb5\xd0\xbc",             // ием
    "\xd0\xb8\xd1\x8f\xd1\x85",             // иях
    "\xd0\xb5\xd0\xb2",                     // ев
    "\xd0\xbe\xd0\xb2",                     // ов
    "\xd0\xb8\xd0\xb5",                     // ие
    "\xd1\x8c\xd0\xb5",                     // ье
    "\xd0\xb5\xd0\xb8",                     // еи
    "\xd0\xb8\xd0\xb8",                     // ии
    "\xd0\xb5\xd0\xb9",                     // ей
    "\xd0\xbe\xd0\xb9",                     // ой
    "\xd0\xb8\xd0\xb9",                     // ий
    "\xd1\x8f\xd0\xbc",                     // ям
    "\xd0\xb5\xd0\xbc",                     // ем
    "\xd0\xb0\xd0\xbc",                     // ам
    "\xd0\xbe\xd0\xbc",                     // ом
    "\xd0\xb0\xd1\x85",                     // ах
    "\xd1\x8f\xd1\x85",                     // ях
    "\xd0\xb8\xd1\x8e",                     // ию
    "\xd1\x8c\xd1\x8e",                     // ью
    "\xd0\xb8\xd1\x8f",                     // ия
    "\xd1\x8c\xd1\x8f",                     // ья
    "\xd0\xb0",                             // а
    "\xd0\xb5",                             // е
    "\xd0\xb8",                             // и
    "\xd0\xb9",                             // й
    "\xd0\xbe",                             // о
    "\xd1\x83",                             // у
    "\xd1\x8b",                             // ы
    "\xd1\x8c",                             // ь
    "\xd1\x8e",                             // ю
    "\xd1\x8f",                             // я
    0
};

static const char* deriv_suf[] = {
    "\xd0\xbe\xd1\x81\xd1\x82\xd1\x8c", // ость
    "\xd0\xbe\xd1\x81\xd1\x82",         // ост
    0
};

static const char* super_suf[] = {
    "\xd0\xb5\xd0\xb9\xd1\x88\xd0\xb5", // ейше
    "\xd0\xb5\xd0\xb9\xd1\x88",         // ейш
    0
};

std::string Stemmer::stem(const std::string& word) {
    if (word.size() < 4) return word; // Too short to stem

    int len = word.size();
    char* w = new char[len + 1];
    memcpy(w, word.c_str(), len + 1);

    int rv = findRV(w, len);
    int r2 = findR2(w, len);

    bool found = tryRemove(w, &len, rv, perf_ger_g2, false);
    if (!found) found = tryRemove(w, &len, rv, perf_ger_g1, true);

    if (!found) {
        tryRemove(w, &len, rv, refl_suf, false);

        found = tryRemove(w, &len, rv, adj_suf, false);
        if (found) {
            if (!tryRemove(w, &len, rv, part_g2, false))
                tryRemove(w, &len, rv, part_g1, true);
        }

        if (!found) {
            found = tryRemove(w, &len, rv, verb_g2, false);
            if (!found) found = tryRemove(w, &len, rv, verb_g1, true);
        }

        if (!found) {
            tryRemove(w, &len, rv, noun_suf, false);
        }
    }

    if (len > rv && len >= 2) {
        unsigned char c1 = w[len - 2];
        unsigned char c2 = w[len - 1];
        // и: D0 B8
        if (c1 == 0xD0 && c2 == 0xB8) {
            len -= 2;
            w[len] = '\0';
        }
    }

    r2 = findR2(w, len);
    tryRemove(w, &len, r2, deriv_suf, false);

    tryRemove(w, &len, rv, super_suf, false);

    //(нн -> н)
    if (len >= 4) {
        unsigned char a1 = w[len - 4], a2 = w[len - 3];
        unsigned char b1 = w[len - 2], b2 = w[len - 1];
        // н: D0 BD
        if (a1 == 0xD0 && a2 == 0xBD && b1 == 0xD0 && b2 == 0xBD) {
            len -= 2;
            w[len] = '\0';
        }
    }

    // Remove ь at end of RV
    rv = findRV(w, len);
    if (len > rv && len >= 2) {
        unsigned char c1 = w[len - 2];
        unsigned char c2 = w[len - 1];
        if (c1 == 0xD1 && c2 == 0x8C) {
            len -= 2;
            w[len] = '\0';
        }
    }

    std::string result(w, len);
    delete[] w;
    return result;
}
