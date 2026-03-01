package main

import (
	"flag"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os/exec"
	"strings"
)

var (
	enginePath = flag.String("engine", "../engine/engine", "Path to C++ engine binary")
	indexPath  = flag.String("index", "../engine/index.bin", "Path to index file")
	port       = flag.Int("port", 8080, "HTTP port")
)

const htmlTemplate = `<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <title>IR Search Engine</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
               background: #f5f5f5; color: #333; }
        .container { max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { text-align: center; margin: 40px 0 20px; color: #1a73e8; }
        .search-form { display: flex; gap: 10px; margin: 20px 0; }
        .search-form input[type=text] {
            flex: 1; padding: 12px 16px; font-size: 16px;
            border: 2px solid #ddd; border-radius: 8px; outline: none;
        }
        .search-form input[type=text]:focus { border-color: #1a73e8; }
        .search-form button {
            padding: 12px 24px; font-size: 16px; background: #1a73e8;
            color: white; border: none; border-radius: 8px; cursor: pointer;
        }
        .search-form button:hover { background: #1557b0; }
        .hint { color: #666; font-size: 14px; margin-bottom: 20px; }
        .results { margin-top: 20px; }
        .result-count { color: #666; margin-bottom: 15px; }
        .result-item { background: white; padding: 15px; margin-bottom: 10px;
                       border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
        .result-item .title { font-size: 18px; }
        .result-item .title a { color: #1a73e8; text-decoration: none; }
        .result-item .title a:hover { text-decoration: underline; }
        .result-item .docid { font-size: 12px; color: #999; }
    </style>
</head>
<body>
<div class="container">
    <h1>IR Search Engine</h1>
    <form class="search-form" method="get" action="/">
        <input type="text" name="q" value="{{.Query}}" placeholder="Введите запрос..." autofocus>
        <button type="submit">Поиск</button>
    </form>
    <div class="hint">Поддерживаются операторы: AND, OR, NOT, скобки. Пример: кошка AND собака</div>
    {{.ResultsHTML}}
</div>
</body>
</html>`

type result struct {
	DocID string
	Title string
	URL   string
}

type pageData struct {
	Query       string
	ResultsHTML template.HTML
}

func runSearch(query string) (int, []result) {
	cmd := exec.Command(*enginePath, "search", "-index", *indexPath, "-query", query)
	out, err := cmd.CombinedOutput()
	if err != nil {
		log.Printf("Engine error: %v, output: %s", err, out)
		return 0, nil
	}

	return parseResults(string(out))
}

func parseResults(output string) (int, []result) {
	lines := strings.Split(strings.TrimSpace(output), "\n")
	var count int
	var results []result

	for _, line := range lines {
		if strings.HasPrefix(line, "Results:") {
			fmt.Sscanf(line, "Results: %d documents", &count)
			continue
		}
		parts := strings.SplitN(line, "\t", 3)
		if len(parts) >= 3 {
			docID := strings.TrimSpace(parts[0])
			title := parts[1]
			url := parts[2]
			if docID != "" && title != "" {
				results = append(results, result{DocID: docID, Title: title, URL: url})
			}
		}
	}

	return count, results
}

func handler(w http.ResponseWriter, r *http.Request) {
	query := r.URL.Query().Get("q")

	var resultsHTML string
	if query != "" {
		count, results := runSearch(query)
		var sb strings.Builder
		sb.WriteString(fmt.Sprintf(`<div class="results"><div class="result-count">Найдено: %d документов</div>`, count))
		for _, res := range results {
			titleEsc := template.HTMLEscapeString(res.Title)
			urlEsc := template.HTMLEscapeString(res.URL)
			titleHTML := titleEsc
			if res.URL != "" {
				titleHTML = fmt.Sprintf(`<a href="%s" target="_blank">%s</a>`, urlEsc, titleEsc)
			}
			sb.WriteString(fmt.Sprintf(`<div class="result-item"><div class="title">%s</div><div class="docid">Document ID: %s</div></div>`,
				titleHTML, template.HTMLEscapeString(res.DocID)))
		}
		sb.WriteString("</div>")
		resultsHTML = sb.String()
	}

	tmpl, _ := template.New("page").Parse(htmlTemplate)
	tmpl.Execute(w, pageData{
		Query:       query,
		ResultsHTML: template.HTML(resultsHTML),
	})
}

func main() {
	flag.Parse()

	http.HandleFunc("/", handler)

	addr := fmt.Sprintf(":%d", *port)
	log.Printf("Search web interface at http://localhost%s", addr)
	log.Printf("Engine: %s, Index: %s", *enginePath, *indexPath)

	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Fatal(err)
	}
}
