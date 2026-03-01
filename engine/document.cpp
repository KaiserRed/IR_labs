#include "document.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


static char* readLine(FILE* fp) {
    int cap = 8192;
    int len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return 0;

    int ch;
    while ((ch = fgetc(fp)) != EOF && ch != '\n') {
        if (len >= cap - 1) {
            cap *= 2;
            char* nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); return 0; }
            buf = nb;
        }
        buf[len++] = (char)ch;
    }

    if (len == 0 && ch == EOF) {
        free(buf);
        return 0;
    }

    buf[len] = '\0';
    return buf;
}

static const char* findField(const char* json, const char* field) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field);
    const char* pos = strstr(json, pattern);
    if (!pos) return 0;
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == ':') pos++;
    return pos;
}

static char* parseJsonString(const char* pos, int* outLen) {
    if (*pos != '"') return 0;
    pos++;

    int cap = 4096;
    int len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return 0;

    while (*pos && *pos != '"') {
        if (len >= cap - 4) {
            cap *= 2;
            char* nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); return 0; }
            buf = nb;
        }

        if (*pos == '\\' && *(pos + 1)) {
            pos++;
            switch (*pos) {
                case '"':  buf[len++] = '"'; break;
                case '\\': buf[len++] = '\\'; break;
                case 'n':  buf[len++] = '\n'; break;
                case 't':  buf[len++] = '\t'; break;
                case 'r':  buf[len++] = '\r'; break;
                case '/':  buf[len++] = '/'; break;
                case 'u': {
                    unsigned cp = 0;
                    for (int i = 0; i < 4 && pos[1+i]; i++) {
                        char c = pos[1+i];
                        cp <<= 4;
                        if (c >= '0' && c <= '9') cp |= (c - '0');
                        else if (c >= 'a' && c <= 'f') cp |= (c - 'a' + 10);
                        else if (c >= 'A' && c <= 'F') cp |= (c - 'A' + 10);
                    }
                    pos += 4;
                    if (cp < 0x80) {
                        buf[len++] = (char)cp;
                    } else if (cp < 0x800) {
                        buf[len++] = (char)(0xC0 | (cp >> 6));
                        buf[len++] = (char)(0x80 | (cp & 0x3F));
                    } else {
                        buf[len++] = (char)(0xE0 | (cp >> 12));
                        buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        buf[len++] = (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: buf[len++] = *pos; break;
            }
        } else {
            buf[len++] = *pos;
        }
        pos++;
    }

    buf[len] = '\0';
    if (outLen) *outLen = len;
    return buf;
}

static int parseJsonInt(const char* pos) {
    return atoi(pos);
}

DynArray<RawDocument> loadDocuments(const char* jsonlPath) {
    DynArray<RawDocument> docs;

    FILE* fp = fopen(jsonlPath, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", jsonlPath);
        return docs;
    }

    char* line;
    int count = 0;
    while ((line = readLine(fp)) != 0) {
        RawDocument doc;

        const char* p;

        p = findField(line, "page_id");
        if (p) doc.page_id = parseJsonInt(p);

        p = findField(line, "title");
        if (p) doc.title = parseJsonString(p, 0);

        p = findField(line, "url");
        if (p) doc.url = parseJsonString(p, 0);

        p = findField(line, "html");
        if (p) doc.html = parseJsonString(p, 0);

        if (doc.title && doc.html) {
            docs.push(doc);
            doc.title = 0;
            doc.url = 0;
            doc.html = 0;
        }

        count++;
        if (count % 5000 == 0)
            fprintf(stderr, "Loaded %d documents...\n", count);

        free(line);
    }

    fclose(fp);
    fprintf(stderr, "Loaded %d documents total\n", docs.size());
    return docs;
}

char* stripHTML(const char* html) {
    if (!html) return 0;

    int hlen = strlen(html);
    char* out = (char*)malloc(hlen + 1);
    if (!out) return 0;

    int olen = 0;
    int inTag = 0;
    int inScript = 0;
    int inStyle = 0;

    for (int i = 0; i < hlen; i++) {
        if (html[i] == '<') {
            inTag = 1;
            if (i + 7 < hlen) {
                if (strncasecmp(html + i, "<script", 7) == 0) inScript = 1;
                if (strncasecmp(html + i, "<style", 6) == 0) inStyle = 1;
            }
            if (i + 8 < hlen) {
                if (strncasecmp(html + i, "</script", 8) == 0) inScript = 0;
            }
            if (i + 7 < hlen) {
                if (strncasecmp(html + i, "</style", 7) == 0) inStyle = 0;
            }
            continue;
        }

        if (html[i] == '>') {
            inTag = 0;
            if (olen > 0 && out[olen - 1] != ' ')
                out[olen++] = ' ';
            continue;
        }

        if (inTag || inScript || inStyle) continue;

        if (html[i] == '&') {
            if (strncmp(html + i, "&amp;", 5) == 0)       { out[olen++] = '&'; i += 4; continue; }
            if (strncmp(html + i, "&lt;", 4) == 0)        { out[olen++] = '<'; i += 3; continue; }
            if (strncmp(html + i, "&gt;", 4) == 0)        { out[olen++] = '>'; i += 3; continue; }
            if (strncmp(html + i, "&quot;", 6) == 0)      { out[olen++] = '"'; i += 5; continue; }
            if (strncmp(html + i, "&nbsp;", 6) == 0)      { out[olen++] = ' '; i += 5; continue; }
            if (strncmp(html + i, "&apos;", 6) == 0)      { out[olen++] = '\''; i += 5; continue; }
            if (html[i + 1] == '#') {
                const char* end = strchr(html + i, ';');
                if (end && end - (html + i) < 12) {
                    i = (int)(end - html);
                    out[olen++] = ' ';
                    continue;
                }
            }
        }

        out[olen++] = html[i];
    }

    out[olen] = '\0';
    return out;
}
