package main

import (
	"hash/fnv"
	"log"
	"regexp"
	"strconv"
	"strings"
	"sync"

	"golang.org/x/net/html"
)

const animalJournalBase = "https://animaljournal.ru"

type AnimalJournalClient struct {
	http *HTTPClient
}

func NewAnimalJournalClient(http *HTTPClient) *AnimalJournalClient {
	return &AnimalJournalClient{http: http}
}

// pageLinkRe matches pagination links like /page/2
var pageLinkRe = regexp.MustCompile(`href="(/page/(\d+))"`)

func hashURL(s string) int {
	h := fnv.New32a()
	h.Write([]byte(s))
	v := int(h.Sum32())
	if v < 0 {
		v = -v
	}
	return v
}

func (c *AnimalJournalClient) FetchListPage(pageNum int) ([]ArticleLink, []int, error) {
	var listURL string
	if pageNum <= 1 {
		listURL = animalJournalBase + "/"
	} else {
		listURL = animalJournalBase + "/page/" + strconv.Itoa(pageNum)
	}

	body, err := c.http.Get(listURL)
	if err != nil {
		return nil, nil, err
	}

	return c.parseListPage(string(body))
}

type ArticleLink struct {
	Path  string
	Title string
}

func (c *AnimalJournalClient) parseListPage(htmlContent string) ([]ArticleLink, []int, error) {
	var articles []ArticleLink
	var nextPages []int
	seenPaths := make(map[string]bool)

	doc, err := html.Parse(strings.NewReader(htmlContent))
	if err != nil {
		return nil, nil, err
	}

	var f func(*html.Node)
	f = func(n *html.Node) {
		if n.Type == html.ElementNode && n.Data == "a" {
			var href, title string
			for _, attr := range n.Attr {
				if attr.Key == "href" {
					href = attr.Val
				}
			}
			if href == "" {
				goto children
			}

			if strings.HasPrefix(href, "/article/") {
				if !seenPaths[href] {
					seenPaths[href] = true
					title = extractText(n)
					articles = append(articles, ArticleLink{Path: href, Title: strings.TrimSpace(title)})
				}
			} else if strings.HasPrefix(href, "/page/") {
				if num, err := strconv.Atoi(strings.TrimPrefix(href, "/page/")); err == nil && num > 1 {
					nextPages = append(nextPages, num)
				}
			}
		}
	children:
		for child := n.FirstChild; child != nil; child = child.NextSibling {
			f(child)
		}
	}
	f(doc)

	return articles, nextPages, nil
}

func extractText(n *html.Node) string {
	if n.Type == html.TextNode {
		return n.Data
	}
	var s string
	for c := n.FirstChild; c != nil; c = c.NextSibling {
		s += extractText(c)
	}
	return s
}

func (c *AnimalJournalClient) FetchArticle(path string) (string, string, error) {
	fullURL := animalJournalBase + path
	body, err := c.http.Get(fullURL)
	if err != nil {
		return "", "", err
	}

	title, htmlContent := c.parseArticlePage(string(body))
	return title, htmlContent, nil
}

func (c *AnimalJournalClient) parseArticlePage(htmlContent string) (string, string) {
	doc, err := html.Parse(strings.NewReader(htmlContent))
	if err != nil {
		return "", htmlContent
	}

	var title string
	var f func(*html.Node)
	f = func(n *html.Node) {
		if n.Type == html.ElementNode && n.Data == "title" && n.FirstChild != nil {
			title = strings.TrimSpace(n.FirstChild.Data)
			title = strings.TrimSuffix(title, " — AnimalJournal.ru")
			title = strings.TrimSuffix(title, " - AnimalJournal.ru")
		}
		for c := n.FirstChild; c != nil; c = c.NextSibling {
			f(c)
		}
	}
	f(doc)

	return title, htmlContent
}

func (c *AnimalJournalClient) GetMaxPageFromPagination(htmlContent string) int {
	maxPage := 1
	for _, m := range pageLinkRe.FindAllStringSubmatch(htmlContent, -1) {
		if len(m) >= 3 {
			if n, err := strconv.Atoi(m[2]); err == nil && n > maxPage {
				maxPage = n
			}
		}
	}
	return maxPage
}

func (c *AnimalJournalClient) Crawl(db *MongoDB, workers int, onProgress func(), canContinue func() bool) error {
	visited := make(map[string]bool)
	toVisit := []int{1}
	seenPages := make(map[int]bool)

	for len(toVisit) > 0 && canContinue() {
		pageNum := toVisit[0]
		toVisit = toVisit[1:]
		if seenPages[pageNum] {
			continue
		}
		seenPages[pageNum] = true

		articles, nextPages, err := c.FetchListPage(pageNum)
		if err != nil {
			log.Printf("[animaljournal] Error fetching page %d: %v", pageNum, err)
			continue
		}

		log.Printf("[animaljournal] Page %d: %d articles", pageNum, len(articles))

		for _, next := range nextPages {
			if !seenPages[next] {
				toVisit = append(toVisit, next)
			}
		}

		if workers < 1 {
			workers = 1
		}
		sem := make(chan struct{}, workers)
		var wg sync.WaitGroup

		for _, art := range articles {
			if !canContinue() {
				wg.Wait()
				return nil
			}
			if visited[art.Path] {
				continue
			}
			visited[art.Path] = true

			fullURL := animalJournalBase + art.Path
			pageID := hashURL(fullURL)
			if db.DocumentExists(pageID) {
				continue
			}

			wg.Add(1)
			sem <- struct{}{}
			go func(art ArticleLink) {
				defer wg.Done()
				defer func() { <-sem }()

				_, htmlContent, err := c.FetchArticle(art.Path)
				if err != nil {
					log.Printf("[animaljournal] Error fetching %s: %v", art.Path, err)
					return
				}

				title := art.Title
				if title == "" {
					title, _ = c.parseArticlePage(htmlContent)
				}
				if title == "" {
					title = art.Path
				}

				canonURL := CanonicalizeURL(animalJournalBase + art.Path)
				if canonURL == "" {
					canonURL = animalJournalBase + art.Path
				}

				doc := &Document{
					PageID:   hashURL(animalJournalBase + art.Path),
					Title:    title,
					URL:      canonURL,
					HTML:     htmlContent,
					Category: "animaljournal",
				}

				if saved, err := db.SaveDocument(doc); err != nil {
					log.Printf("[animaljournal] Save error %s: %v", art.Path, err)
				} else if saved {
					onProgress()
				}
			}(art)
		}
		wg.Wait()
	}

	return nil
}
