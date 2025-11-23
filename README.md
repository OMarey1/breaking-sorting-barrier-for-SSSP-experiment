# Breaking Sorting Barrier for SSSP Experiment

This repository contains a small C++ benchmark that compares two shortest path algorithms on the same directed graph:

1. **Dijkstra (binary heap)** � classic implementation.
2. **Breaking Sorting Barrier SSSP** � a d-ary/radix-heap style priority queue that avoids the explicit sorting bottleneck for non-negative edge weights.

The graph input is expected in the form `(from, to, distance)` per line, using zero-based node indices.

## Build

```bash
g++ -std=c++17 -O2 -o sssp_benchmark src/main.cpp
```

## Run

Supply the input file path, the source node, and optionally how many times to repeat each algorithm (default: 1):

```bash
./sssp_benchmark sample_graph.txt 0 5
```

## Generating a much larger graph

To highlight the performance gap between the binary-heap and radix-heap implementations, create a substantially larger dataset with the helper script:

```bash
python scripts/generate_random_graph.py large_graph.txt --nodes 50000 --edges 300000 --max-weight 1000 --seed 123
```

Then build and run the benchmark against the new file (the higher the repetition count, the easier it is to see stable timings):

```bash
g++ -std=c++17 -O2 -o sssp_benchmark main.cpp
./sssp_benchmark large_graph.txt 0 10
```

Adjust the flags to push the graph size further if your machine has enough memory; the script enforces basic sanity checks so you do not accidentally request an impossible density.

The program reports the average and best execution time (in milliseconds) of both algorithms across the requested runs, checks that their outputs match, and prints a confirmation. Lines that start with `#` in the input are treated as comments and ignored.

## Input format

Each non-comment line must contain three values separated by spaces or tabs:

```text
from to weight
```

- `from`, `to`: zero-based node indices (non-negative)
- `weight`: non-negative integer edge weight

See `sample_graph.txt` for a small example.
