package main

import (
	"encoding/json"
	"fmt"
	"net/url"
)

type WikiClient struct {
	http    *HTTPClient
	apiBase string
}

type CategoryMembersResponse struct {
	Continue struct {
		CMContinue string `json:"cmcontinue"`
	} `json:"continue"`
	Query struct {
		CategoryMembers []CategoryMember `json:"categorymembers"`
	} `json:"query"`
}

type CategoryMember struct {
	PageID int    `json:"pageid"`
	NS     int    `json:"ns"`
	Title  string `json:"title"`
}

type ParseResponse struct {
	Parse struct {
		Title  string `json:"title"`
		PageID int    `json:"pageid"`
		Text   struct {
			Content string `json:"*"`
		} `json:"text"`
	} `json:"parse"`
}

func NewWikiClient(http *HTTPClient, apiBase string) *WikiClient {
	return &WikiClient{http: http, apiBase: apiBase}
}

func (w *WikiClient) ListCategoryMembers(category string, cmcontinue string, limit int) (*CategoryMembersResponse, error) {
	params := url.Values{}
	params.Set("action", "query")
	params.Set("list", "categorymembers")
	params.Set("cmtitle", category)
	params.Set("cmlimit", fmt.Sprintf("%d", limit))
	params.Set("cmtype", "page")
	params.Set("format", "json")

	if cmcontinue != "" {
		params.Set("cmcontinue", cmcontinue)
	}

	apiURL := w.apiBase + "?" + params.Encode()

	body, err := w.http.Get(apiURL)
	if err != nil {
		return nil, fmt.Errorf("API request failed: %w", err)
	}

	var resp CategoryMembersResponse
	if err := json.Unmarshal(body, &resp); err != nil {
		return nil, fmt.Errorf("JSON parse failed: %w", err)
	}

	return &resp, nil
}

func (w *WikiClient) GetPageHTML(pageID int) (string, error) {
	params := url.Values{}
	params.Set("action", "parse")
	params.Set("pageid", fmt.Sprintf("%d", pageID))
	params.Set("format", "json")
	params.Set("prop", "text")

	apiURL := w.apiBase + "?" + params.Encode()

	body, err := w.http.Get(apiURL)
	if err != nil {
		return "", fmt.Errorf("API request failed: %w", err)
	}

	var resp ParseResponse
	if err := json.Unmarshal(body, &resp); err != nil {
		return "", fmt.Errorf("JSON parse failed: %w", err)
	}

	return resp.Parse.Text.Content, nil
}
