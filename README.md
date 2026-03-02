# Лабораторные работы по информационному поиску

Поисковая система для корпуса статей русской Википедии (животные, растения, грибы) и animaljournal.

## Запуск

### 1. Crawler (Go)

```bash
cd crawler
go mod tidy
go build -o crawler

# Запуск краулера (категории настраиваются в config.yaml)
./crawler -config config.yaml

# Экспорт из MongoDB в JSONL для C++ движка
./crawler -config config.yaml -export ../documents.jsonl
```

### 2. Engine (C++)

```bash
cd engine
make

# Построение индекса из JSONL
./engine build -input ../documents.jsonl -index index.bin

# Булев поиск (одиночный запрос)
./engine search -index index.bin -query "кошка AND собака"

# Интерактивный поиск (stdin → stdout)
./engine search -index index.bin

# Экспорт статистики Ципфа
./engine zipf -index index.bin -output zipf.csv
```

### 3. Графики и веб-интерфейс

```bash
# Построение графика закона Ципфа (TF — частота вхождений)
python3 scripts/zipf_plot.py engine/zipf.csv zipf_plot.png

# Запуск веб-интерфейса (Go)
cd web && go build -o web && ./web -engine ../engine/engine -index ../engine/index.bin -port 8080
```

## Требования

- Go 1.21+
- MongoDB 4.4+
- g++ с поддержкой C++11
- Python 3.8+ (для графиков: matplotlib)