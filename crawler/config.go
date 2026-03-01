package main

import (
	"os"

	"gopkg.in/yaml.v3"
)

type MongoConfig struct {
	URI        string `yaml:"uri"`
	Database   string `yaml:"database"`
	Collection string `yaml:"collection"`
}

type CrawlerConfig struct {
	Workers        int    `yaml:"workers"`
	RateLimitMs    int    `yaml:"rate_limit_ms"`
	RetryMax       int    `yaml:"retry_max"`
	RetryBaseDelay int    `yaml:"retry_base_delay_ms"`
	UserAgent      string `yaml:"user_agent"`
	APIBase        string `yaml:"api_base"`
	PageLimit      int    `yaml:"page_limit"`
}

type Config struct {
	MongoDB      MongoConfig   `yaml:"mongodb"`
	Crawler      CrawlerConfig `yaml:"crawler"`
	Categories   []string      `yaml:"categories"`
	MaxDocuments int           `yaml:"max_documents"`
}

func LoadConfig(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	cfg := &Config{}
	if err := yaml.Unmarshal(data, cfg); err != nil {
		return nil, err
	}

	if cfg.Crawler.Workers == 0 {
		cfg.Crawler.Workers = 5
	}
	if cfg.Crawler.RateLimitMs == 0 {
		cfg.Crawler.RateLimitMs = 200
	}
	if cfg.Crawler.RetryMax == 0 {
		cfg.Crawler.RetryMax = 3
	}
	if cfg.Crawler.RetryBaseDelay == 0 {
		cfg.Crawler.RetryBaseDelay = 1000
	}
	if cfg.Crawler.PageLimit == 0 {
		cfg.Crawler.PageLimit = 500
	}
	if cfg.MaxDocuments == 0 {
		cfg.MaxDocuments = 1000000
	}

	return cfg, nil
}
