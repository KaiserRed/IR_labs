package main

import (
	"fmt"
	"io"
	"math"
	"net/http"
	"sync"
	"time"
)

type HTTPClient struct {
	client    *http.Client
	userAgent string
	rateLimit time.Duration
	retryMax  int
	retryBase time.Duration
	lastReq   time.Time
	mu        sync.Mutex
}

func NewHTTPClient(cfg CrawlerConfig) *HTTPClient {
	return &HTTPClient{
		client: &http.Client{
			Timeout: 30 * time.Second,
			Transport: &http.Transport{
				MaxIdleConns:        100,
				MaxIdleConnsPerHost: 10,
				IdleConnTimeout:     90 * time.Second,
			},
		},
		userAgent: cfg.UserAgent,
		rateLimit: time.Duration(cfg.RateLimitMs) * time.Millisecond,
		retryMax:  cfg.RetryMax,
		retryBase: time.Duration(cfg.RetryBaseDelay) * time.Millisecond,
	}
}

func (c *HTTPClient) Get(url string) ([]byte, error) {
	var lastErr error

	for attempt := 0; attempt <= c.retryMax; attempt++ {
		if attempt > 0 {
			delay := c.retryBase * time.Duration(math.Pow(2, float64(attempt-1)))
			time.Sleep(delay)
		}

		c.mu.Lock()
		elapsed := time.Since(c.lastReq)
		if elapsed < c.rateLimit {
			time.Sleep(c.rateLimit - elapsed)
		}
		c.lastReq = time.Now()
		c.mu.Unlock()

		req, err := http.NewRequest("GET", url, nil)
		if err != nil {
			return nil, fmt.Errorf("request creation failed: %w", err)
		}

		req.Header.Set("User-Agent", c.userAgent)
		req.Header.Set("Accept", "application/json")

		resp, err := c.client.Do(req)
		if err != nil {
			lastErr = err
			continue
		}

		body, err := io.ReadAll(resp.Body)
		resp.Body.Close()

		if err != nil {
			lastErr = err
			continue
		}

		if resp.StatusCode == 429 || resp.StatusCode >= 500 {
			lastErr = fmt.Errorf("HTTP %d", resp.StatusCode)
			continue
		}

		if resp.StatusCode != 200 {
			return nil, fmt.Errorf("HTTP %d: %s", resp.StatusCode, truncate(string(body), 200))
		}

		return body, nil
	}

	return nil, fmt.Errorf("all %d retries failed: %w", c.retryMax+1, lastErr)
}

func truncate(s string, maxLen int) string {
	if len(s) <= maxLen {
		return s
	}
	return s[:maxLen] + "..."
}
