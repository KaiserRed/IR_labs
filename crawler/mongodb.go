package main

import (
	"context"
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"os"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type Document struct {
	ID           primitive.ObjectID `bson:"_id,omitempty" json:"id,omitempty"`
	PageID       int                `bson:"page_id" json:"page_id"`
	Title        string             `bson:"title" json:"title"`
	URL          string             `bson:"url" json:"url"`
	HTML         string             `bson:"html" json:"html"`
	SHA256       string             `bson:"sha256" json:"sha256"`
	Category     string             `bson:"category" json:"category"`
	DownloadedAt time.Time          `bson:"downloaded_at" json:"downloaded_at"`
}

type MongoDB struct {
	client     *mongo.Client
	collection *mongo.Collection
}

func NewMongoDB(cfg MongoConfig) (*MongoDB, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	client, err := mongo.Connect(ctx, options.Client().ApplyURI(cfg.URI))
	if err != nil {
		return nil, err
	}

	if err := client.Ping(ctx, nil); err != nil {
		return nil, err
	}

	collection := client.Database(cfg.Database).Collection(cfg.Collection)

	indexes := []mongo.IndexModel{
		{
			Keys:    bson.D{{Key: "page_id", Value: 1}},
			Options: options.Index().SetUnique(true),
		},
		{
			Keys:    bson.D{{Key: "sha256", Value: 1}},
			Options: options.Index().SetUnique(true),
		},
	}
	collection.Indexes().CreateMany(ctx, indexes)

	return &MongoDB{client: client, collection: collection}, nil
}

func (m *MongoDB) Close() error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	return m.client.Disconnect(ctx)
}

func (m *MongoDB) SaveDocument(doc *Document) (bool, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	hash := sha256.Sum256([]byte(doc.HTML))
	doc.SHA256 = fmt.Sprintf("%x", hash)
	doc.DownloadedAt = time.Now()
	if m.SHA256Exists(doc.SHA256) {
		return false, nil
	}

	filter := bson.M{"page_id": doc.PageID}
	update := bson.M{"$set": doc}
	opts := options.Update().SetUpsert(true)

	_, err := m.collection.UpdateOne(ctx, filter, update, opts)
	if err != nil {
		if mongo.IsDuplicateKeyError(err) {
			return false, nil
		}
		return false, err
	}
	return true, nil
}

func (m *MongoDB) SHA256Exists(sha256 string) bool {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	count, err := m.collection.CountDocuments(ctx, bson.M{"sha256": sha256})
	return err == nil && count > 0
}

func (m *MongoDB) DocumentExists(pageID int) bool {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	count, err := m.collection.CountDocuments(ctx, bson.M{"page_id": pageID})
	return err == nil && count > 0
}

func (m *MongoDB) GetDocumentCount() (int64, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	return m.collection.CountDocuments(ctx, bson.M{})
}

type ExportDocument struct {
	PageID int    `json:"page_id"`
	Title  string `json:"title"`
	URL    string `json:"url"`
	HTML   string `json:"html"`
}

func (m *MongoDB) ExportToJSONL(filename string) (int64, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Minute)
	defer cancel()

	file, err := os.Create(filename)
	if err != nil {
		return 0, err
	}
	defer file.Close()

	cursor, err := m.collection.Find(ctx, bson.M{})
	if err != nil {
		return 0, err
	}
	defer cursor.Close(ctx)

	encoder := json.NewEncoder(file)
	var count int64

	for cursor.Next(ctx) {
		var doc Document
		if err := cursor.Decode(&doc); err != nil {
			continue
		}

		exportDoc := ExportDocument{
			PageID: doc.PageID,
			Title:  doc.Title,
			URL:    doc.URL,
			HTML:   doc.HTML,
		}

		if err := encoder.Encode(exportDoc); err != nil {
			continue
		}
		count++
	}

	return count, nil
}
