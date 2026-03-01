#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "document.h"

static int utf8AlignStart(const char* s, int pos) {
    while (pos > 0 && (unsigned char)s[pos] >= 0x80 && (unsigned char)s[pos] < 0xC0)
        pos--;
    return pos;
}

static int utf8AlignEnd(const char* s, int pos) {
    while (pos > 0 && (unsigned char)s[pos - 1] >= 0x80 && (unsigned char)s[pos - 1] < 0xC0)
        pos--;
    return pos;
}

static char* extractSnippet(const char* text, const char* textLower, const char* token, int maxBefore = 50, int maxAfter = 80) {
    const char* pos = strstr(textLower, token);
    if (!pos || !token[0]) return 0;
    int start = (int)(pos - textLower);
    int tokenLen = (int)strlen(token);
    int textLen = (int)strlen(text);
    int before = start < maxBefore ? start : maxBefore;
    int after = maxAfter;
    if (start + tokenLen + after > textLen)
        after = textLen - start - tokenLen;
    if (after < 0) after = 0;

    int copyStart = utf8AlignStart(text, start - before);
    int copyEnd = start + tokenLen + after;
    if (copyEnd > textLen) copyEnd = textLen;
    copyEnd = utf8AlignEnd(text, copyEnd);

    if (copyStart >= copyEnd) return 0;

    int len = copyEnd - copyStart;
    char* snippet = new char[len + 4];
    int w = 0;
    if (copyStart > 0) snippet[w++] = '.';
    memcpy(snippet + w, text + copyStart, len);
    w += len;
    snippet[w] = '\0';
    return snippet;
}
#include "tokenizer.h"
#include "stemmer.h"
#include "index.h"
#include "search.h"
#include "zipf.h"

static void printUsage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s build  -input <file.jsonl> [-index index.bin]\n"
        "  %s search -index <index.bin>  [-query \"...\"]\n"
        "  %s zipf   -index <index.bin>  -output <zipf.csv>\n"
        "\n"
        "Commands:\n"
        "  build   Build inverted index from JSONL documents\n"
        "  search  Boolean search (interactive or single query)\n"
        "  zipf    Export Zipf's law statistics to CSV\n",
        prog, prog, prog);
}

static const char* getArg(int argc, char** argv, const char* flag) {
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0)
            return argv[i + 1];
    }
    return 0;
}

static int cmdBuild(int argc, char** argv) {
    const char* inputFile = getArg(argc, argv, "-input");
    const char* indexFile = getArg(argc, argv, "-index");
    if (!indexFile) indexFile = "index.bin";

    if (!inputFile) {
        fprintf(stderr, "Error: -input is required for build command\n");
        return 1;
    }

    fprintf(stderr, "Loading documents from %s...\n", inputFile);
    DynArray<RawDocument> docs = loadDocuments(inputFile);
    if (docs.size() == 0) {
        fprintf(stderr, "No documents loaded\n");
        return 1;
    }

    InvertedIndex index;
    Tokenizer tokenizer;
    Stemmer stemmer;

    fprintf(stderr, "Building index...\n");
    for (int i = 0; i < docs.size(); i++) {
        index.addDocument(docs[i].page_id, docs[i].title, docs[i].url);

        char* text = stripHTML(docs[i].html);
        if (!text) continue;

        std::vector<std::string> tokens = tokenizer.tokenize(std::string(text));
        std::string textLower = tokenizer.toLowerUtf8(std::string(text));

        DynArray<const char*> terms;
        DynArray<int> counts;
        DynArray<const char*> firstTokens;
        for (size_t j = 0; j < tokens.size(); j++) {
            std::string stemmed = stemmer.stem(tokens[j]);
            const char* t = stemmed.c_str();
            int found = -1;
            for (int k = 0; k < terms.size(); k++) {
                if (strcmp(terms[k], t) == 0) { found = k; break; }
            }
            if (found >= 0) {
                counts[found]++;
            } else {
                int len = stemmed.size();
                char* copy = new char[len + 1];
                memcpy(copy, t, len + 1);
                terms.push(copy);
                counts.push(1);
                firstTokens.push(tokens[j].c_str());
            }
        }
        for (int k = 0; k < terms.size(); k++) {
            char* snippet = 0;
            if (firstTokens[k]) {
                snippet = extractSnippet(text, textLower.c_str(), firstTokens[k]);
            }
            index.addTerm(docs[i].page_id, terms[k], counts[k], snippet);
            delete[] terms[k];
            delete[] snippet;
        }
        free(text);

        if ((i + 1) % 1000 == 0)
            fprintf(stderr, "Indexed %d/%d documents\n", i + 1, docs.size());
    }

    index.finalize();
    index.save(indexFile);

    return 0;
}

static int cmdSearch(int argc, char** argv) {
    const char* indexFile = getArg(argc, argv, "-index");
    const char* queryStr  = getArg(argc, argv, "-query");
    if (!indexFile) indexFile = "index.bin";

    InvertedIndex index;
    index.load(indexFile);

    if (index.getDocCount() == 0) {
        fprintf(stderr, "Empty index or failed to load\n");
        return 1;
    }

    BooleanSearch search(&index);
    const DynArray<DocInfo>& docs = index.getDocuments();

    std::string firstTerm = search.getFirstTerm(queryStr ? queryStr : "");
    auto printResults = [&](const DynArray<int>& results, int limit) {
        for (int i = 0; i < results.size() && i < limit; i++) {
            for (int d = 0; d < docs.size(); d++) {
                if (docs[d].docId == results[i]) {
                    const char* title = docs[d].title ? docs[d].title : "?";
                    const char* url = docs[d].url ? docs[d].url : "";
                    printf("%d\t%s\t%s\n", docs[d].docId, title, url);
                    break;
                }
            }
        }
    };

    if (queryStr) {
        DynArray<int> results = search.search(queryStr);
        printf("Query: %s\n", queryStr);
        printf("Results: %d documents\n", results.size());
        printResults(results, 20);
        if (results.size() > 20)
            printf("  ... and %d more\n", results.size() - 20);
    } else {
        fprintf(stderr, "Interactive search mode. Enter queries (one per line), Ctrl+D to exit.\n");
        char line[4096];
        while (fgets(line, sizeof(line), stdin)) {
            int len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
            if (strlen(line) == 0) continue;

            firstTerm = search.getFirstTerm(line);
            DynArray<int> results = search.search(line);
            printf("Query: %s\n", line);
            printf("Results: %d documents\n", results.size());
            printResults(results, 20);
            if (results.size() > 20)
                printf("  ... and %d more\n", results.size() - 20);
            printf("\n");
        }
    }

    return 0;
}

static int cmdZipf(int argc, char** argv) {
    const char* indexFile  = getArg(argc, argv, "-index");
    const char* outputFile = getArg(argc, argv, "-output");
    if (!indexFile)  indexFile  = "index.bin";
    if (!outputFile) outputFile = "zipf.csv";

    InvertedIndex index;
    index.load(indexFile);

    if (index.getDocCount() == 0) {
        fprintf(stderr, "Empty index or failed to load\n");
        return 1;
    }

    ZipfAnalyzer zipf;
    zipf.analyze(&index);
    zipf.exportCSV(outputFile);

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "build") == 0)  return cmdBuild(argc, argv);
    if (strcmp(cmd, "search") == 0) return cmdSearch(argc, argv);
    if (strcmp(cmd, "zipf") == 0)   return cmdZipf(argc, argv);

    fprintf(stderr, "Unknown command: %s\n", cmd);
    printUsage(argv[0]);
    return 1;
}
