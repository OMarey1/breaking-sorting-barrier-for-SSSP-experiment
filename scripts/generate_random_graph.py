#!/usr/bin/env python3
"""Generate large random SSSP benchmark graphs.

The output format matches the benchmark input: one directed edge per line as
"from to weight" with zero-based node ids. Lines that start with # are treated as
comments by the benchmark and are used here for metadata.
"""

import argparse
import pathlib
import random
import sys
from typing import Iterable, Tuple

Edge = Tuple[int, int, int]


def _validate_args(nodes: int, edges: int, max_weight: int) -> None:
    if nodes < 2:
        raise ValueError("nodes must be >= 2 to build an interesting graph")
    if edges < nodes - 1:
        raise ValueError("edges must be at least nodes-1 to keep the graph connected")
    max_possible = nodes * (nodes - 1)
    if edges > max_possible:
        raise ValueError(f"edges must be <= {max_possible} for {nodes} nodes (no self-loops)")
    if max_weight <= 0:
        raise ValueError("max-weight must be positive")


def generate_edges(nodes: int, edges: int, max_weight: int, rng: random.Random) -> Iterable[Edge]:
    """Generate a directed graph with at least a simple backbone path."""
    used = set()
    # Create a simple path to guarantee reachability from node 0.
    for u in range(nodes - 1):
        v = u + 1
        w = rng.randint(1, max_weight)
        used.add((u, v))
        yield (u, v, w)

    remaining = edges - (nodes - 1)
    while remaining > 0:
        u = rng.randrange(nodes)
        v = rng.randrange(nodes)
        if u == v:
            continue
        key = (u, v)
        if key in used:
            continue
        w = rng.randint(1, max_weight)
        used.add(key)
        yield (u, v, w)
        remaining -= 1


def main(argv: Iterable[str]) -> int:
    parser = argparse.ArgumentParser(description="Generate a large random graph file for the SSSP benchmark.")
    parser.add_argument("output", type=pathlib.Path, help="Path to write the generated graph (text file)")
    parser.add_argument("--nodes", type=int, default=50000, help="Number of nodes to generate (default: 50000)")
    parser.add_argument("--edges", type=int, default=300000, help="Number of directed edges (default: 300000)")
    parser.add_argument("--max-weight", type=int, default=1000, help="Maximum edge weight (default: 1000)")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility (default: 42)")
    args = parser.parse_args(argv)

    _validate_args(args.nodes, args.edges, args.max_weight)

    rng = random.Random(args.seed)
    edges = list(generate_edges(args.nodes, args.edges, args.max_weight, rng))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="ascii") as fh:
        fh.write(f"# Random graph generated with nodes={args.nodes}, edges={args.edges}, max_weight={args.max_weight}, seed={args.seed}\n")
        for u, v, w in edges:
            fh.write(f"{u} {v} {w}\n")

    print(f"Wrote {len(edges)} edges covering {args.nodes} nodes to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
