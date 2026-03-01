#!/usr/bin/env python3
"""
Построение графика закона Ципфа из CSV-файла, сгенерированного engine.
Использование: python3 zipf_plot.py zipf.csv [output.png]
"""

import sys
import csv
import math

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 zipf_plot.py <zipf.csv> [output.png]")
        sys.exit(1)

    csv_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "zipf_plot.png"

    ranks = []
    freqs = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            ranks.append(int(row['rank']))
            freqs.append(int(row['frequency']))

    if not ranks:
        print("No data found in CSV")
        sys.exit(1)

    try:
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(figsize=(8, 6))

        log_ranks = [math.log10(r) for r in ranks]
        log_freqs = [math.log10(f) if f > 0 else 0 for f in freqs]
        ax.plot(log_ranks, log_freqs, 'b-', linewidth=0.5)
        ax.set_xlabel('log₁₀(Ранг)')
        ax.set_ylabel('log₁₀(Частота)')
        ax.set_title('Закон Ципфа (лог-лог шкала)')
        ax.grid(True, alpha=0.3)

        if len(log_ranks) > 1:
            c = log_freqs[0]
            ideal = [c - lr for lr in log_ranks]
            ax.plot(log_ranks, ideal, 'g--', linewidth=1, alpha=0.7, label='Идеальный Ципф (наклон -1)')
            ax.legend()

        plt.tight_layout()
        plt.savefig(output_path, dpi=150)
        print(f"Plot saved to {output_path}")

        # Stats
        print(f"\nСтатистика:")
        print(f"  Всего уникальных терминов: {len(ranks)}")
        print(f"  Максимальная частота: {freqs[0]} (термин на 1-м месте)")
        print(f"  Медианная частота: {freqs[len(freqs)//2]}")
        print(f"  Hapax legomena (частота=1): {sum(1 for f in freqs if f == 1)}")

    except ImportError:
        print("matplotlib not installed. Saving data summary instead.")
        print(f"\nTop-20 terms:")
        with open(csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for i, row in enumerate(reader):
                if i >= 20:
                    break
                print(f"  {row['rank']:>5}  {row['frequency']:>8}  {row['term']}")
        print(f"\nTotal unique terms: {len(ranks)}")

if __name__ == "__main__":
    main()
