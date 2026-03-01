package main

import (
	"log"
	"sync"
	"sync/atomic"
)

type Crawler struct {
	config     *Config
	db         *MongoDB
	wiki       *WikiClient
	animal     *AnimalJournalClient
	http       *HTTPClient
	downloaded int64
}

func NewCrawler(cfg *Config, db *MongoDB) *Crawler {
	httpClient := NewHTTPClient(cfg.Crawler)
	return &Crawler{
		config: cfg,
		db:     db,
		http:   httpClient,
		wiki:   NewWikiClient(httpClient, cfg.Crawler.APIBase),
		animal: NewAnimalJournalClient(httpClient),
	}
}

func (c *Crawler) Run() error {
	if c.config.AnimalJournal {
		log.Printf("=== Processing source: animaljournal.ru ===")
		if err := c.crawlAnimalJournal(); err != nil {
			log.Printf("Error crawling animaljournal: %v", err)
		}
		if atomic.LoadInt64(&c.downloaded) >= int64(c.config.MaxDocuments) {
			return nil
		}
	}

	for _, category := range c.config.Categories {
		if atomic.LoadInt64(&c.downloaded) >= int64(c.config.MaxDocuments) {
			break
		}
		log.Printf("=== Processing category: %s ===", category)
		if err := c.crawlCategory(category); err != nil {
			log.Printf("Error crawling category %s: %v", category, err)
		}
	}
	return nil
}

func (c *Crawler) crawlAnimalJournal() error {
	return c.animal.Crawl(c.db, c.config.Crawler.Workers,
		func() {
			count := atomic.AddInt64(&c.downloaded, 1)
			if count%50 == 0 {
				dbCount, _ := c.db.GetDocumentCount()
				log.Printf("Progress: %d downloaded this run, %d total in DB", count, dbCount)
			}
		},
		func() bool { return atomic.LoadInt64(&c.downloaded) < int64(c.config.MaxDocuments) },
	)
}

func (c *Crawler) crawlCategory(category string) error {
	cmcontinue := ""
	pageNum := 0

	for {
		if atomic.LoadInt64(&c.downloaded) >= int64(c.config.MaxDocuments) {
			return nil
		}

		pageNum++
		resp, err := c.wiki.ListCategoryMembers(category, cmcontinue, c.config.Crawler.PageLimit)
		if err != nil {
			return err
		}

		members := resp.Query.CategoryMembers
		log.Printf("[%s] Page %d: got %d members", category, pageNum, len(members))

		c.processMembers(members, category)

		if resp.Continue.CMContinue == "" {
			log.Printf("[%s] No more pages, finished", category)
			break
		}
		cmcontinue = resp.Continue.CMContinue
	}

	return nil
}

func (c *Crawler) processMembers(members []CategoryMember, category string) {
	var wg sync.WaitGroup
	sem := make(chan struct{}, c.config.Crawler.Workers)

	for _, member := range members {
		if atomic.LoadInt64(&c.downloaded) >= int64(c.config.MaxDocuments) {
			break
		}

		if member.NS != 0 {
			continue
		}

		if c.db.DocumentExists(member.PageID) {
			continue
		}

		wg.Add(1)
		sem <- struct{}{}

		go func(m CategoryMember) {
			defer wg.Done()
			defer func() { <-sem }()

			saved, err := c.fetchAndSave(m, category)
			if err != nil {
				log.Printf("Error fetching %q (pageid=%d): %v", m.Title, m.PageID, err)
				return
			}
			if !saved {
				return // duplicate content (SHA256), skipped
			}

			count := atomic.AddInt64(&c.downloaded, 1)
			if count%100 == 0 {
				dbCount, _ := c.db.GetDocumentCount()
				log.Printf("Progress: %d downloaded this run, %d total in DB", count, dbCount)
			}
		}(member)
	}

	wg.Wait()
}

func (c *Crawler) fetchAndSave(member CategoryMember, category string) (bool, error) {
	html, err := c.wiki.GetPageHTML(member.PageID)
	if err != nil {
		return false, err
	}

	doc := &Document{
		PageID:   member.PageID,
		Title:    member.Title,
		URL:      CanonicalizeURL(WikiArticleURL(member.Title)),
		HTML:     html,
		Category: category,
	}

	return c.db.SaveDocument(doc)
}
