package main

import (
	"flag"
	"log"
	"os"
)

func main() {
	configPath := flag.String("config", "config.yaml", "Path to config file")
	exportPath := flag.String("export", "", "Export documents from MongoDB to JSONL file")
	flag.Parse()

	cfg, err := LoadConfig(*configPath)
	if err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	db, err := NewMongoDB(cfg.MongoDB)
	if err != nil {
		log.Fatalf("Failed to connect to MongoDB: %v", err)
	}
	defer db.Close()

	if *exportPath != "" {
		log.Printf("Exporting documents to %s...", *exportPath)
		count, err := db.ExportToJSONL(*exportPath)
		if err != nil {
			log.Fatalf("Export failed: %v", err)
		}
		log.Printf("Exported %d documents", count)
		os.Exit(0)
	}

	crawler := NewCrawler(cfg, db)

	log.Printf("Starting crawler...")
	log.Printf("Categories: %v", cfg.Categories)
	log.Printf("Max documents: %d", cfg.MaxDocuments)
	log.Printf("Workers: %d, Rate limit: %dms", cfg.Crawler.Workers, cfg.Crawler.RateLimitMs)

	if err := crawler.Run(); err != nil {
		log.Fatalf("Crawler failed: %v", err)
	}

	count, _ := db.GetDocumentCount()
	log.Printf("Crawler finished. Total documents in DB: %d", count)
}
