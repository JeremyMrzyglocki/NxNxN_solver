import random
import statistics as stats
from collections import Counter
import matplotlib.pyplot as plt

def greedy_two_swap_sort(arr):
    """
    Greedy algorithm:
    Repeatedly find the first index i where arr[i] != target[i] and swap arr[i]
    with the position j where the correct value for i currently resides.
    Returns the number of swaps performed.
    NOTE: This is a greedy strategy; it is not guaranteed to be minimal.
    """
    swaps = 0
    n = len(arr)
    target = sorted(arr)
    arr = arr[:]  # work on a copy

    while arr != target:
        for i in range(n):
            if arr[i] != target[i]:
                correct_val = target[i]
                # find the next occurrence of correct_val after i
                j = arr.index(correct_val, i + 1)
                arr[i], arr[j] = arr[j], arr[i]
                swaps += 1
                break
    return swaps

def run_study(trials=100, seed=42):
    random.seed(seed)

    base_unique = [chr(ord('a') + i) for i in range(24)]  # 'a'..'x'
    base_dupes  = list("aaaa" + "bbbb" + "cccc" + "dddd" + "eeee" + "ffff")

    swaps_unique = []
    swaps_dupes  = []

    for _ in range(trials):
        a1 = base_unique[:]
        random.shuffle(a1)
        swaps_unique.append(greedy_two_swap_sort(a1))

        a2 = base_dupes[:]
        random.shuffle(a2)
        swaps_dupes.append(greedy_two_swap_sort(a2))

    return swaps_unique, swaps_dupes

def print_summary(name, swaps):
    print(f"\n{name}")
    print("-" * len(name))
    print(f"Trials: {len(swaps)}")
    print(f"Average: {stats.mean(swaps):.2f}")
    print(f"Median : {stats.median(swaps):.2f}")
    print(f"Min-Max: {min(swaps)} – {max(swaps)}")
    # simple frequency listing
    freq = sorted(Counter(swaps).items())
    print("Counts :", ", ".join(f"{k}:{v}" for k, v in freq))

def plot_hist(name, swaps):
    # Separate figure per case (no subplots)
    plt.figure(figsize=(7, 5))
    # integer bins from min to max inclusive, aligned to integers
    bins = range(min(swaps), max(swaps) + 2)
    plt.hist(swaps, bins=bins, align="left", edgecolor="black")
    plt.title(name)
    plt.xlabel("Number of greedy swaps")
    plt.ylabel("Frequency")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    # Run 100 trials for each case
    swaps_unique, swaps_dupes = run_study(trials=10000, seed=42)

    # Print summaries
    print_summary("Case 1: Unique letters (a..x)", swaps_unique)
    print_summary("Case 2: Repeated letters (aaaabbbbccccddddeeeeffff)", swaps_dupes)

    # Plot histograms (one figure per case)
    plot_hist("Case 1: Unique letters (a..x)", swaps_unique)
    plot_hist("Case 2: Repeated letters (aaaabbbbccccddddeeeeffff)", swaps_dupes)
