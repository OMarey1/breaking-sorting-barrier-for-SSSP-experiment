#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

struct Edge {
    int to;
    std::uint64_t weight;
};

using Graph = std::vector<std::vector<Edge>>;

struct GraphLoadResult {
    Graph graph;
    int node_count = 0;
};

GraphLoadResult read_graph_from_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open input file: " + path);
    }

    std::vector<std::tuple<int, int, std::uint64_t>> edges;
    std::string line;
    int max_node = -1;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        int from, to;
        long long w_input;
        if (!(iss >> from >> to >> w_input)) {
            throw std::runtime_error("Invalid line in input file: " + line);
        }
        if (from < 0 || to < 0) {
            throw std::runtime_error("Node ids must be non-negative: " + line);
        }
        if (w_input < 0) {
            throw std::runtime_error("Edge weights must be non-negative: " + line);
        }
        std::uint64_t w = static_cast<std::uint64_t>(w_input);
        edges.emplace_back(from, to, w);
        max_node = std::max({ max_node, from, to });
    }

    if (max_node < 0) {
        return {};
    }

    Graph graph(static_cast<std::size_t>(max_node + 1));
    for (const auto& e : edges) {
        int from, to;
        std::uint64_t w;
        std::tie(from, to, w) = e;
        graph[static_cast<std::size_t>(from)].push_back({ to, w });
    }

    return { std::move(graph), max_node + 1 };
}

// Dijkstra using binary heap priority_queue.
std::vector<std::uint64_t> dijkstra(const Graph& graph, int source) {
    const std::uint64_t INF = std::numeric_limits<std::uint64_t>::max();
    std::vector<std::uint64_t> dist(graph.size(), INF);
    dist[source] = 0;

    using P = std::pair<std::uint64_t, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    pq.push({ 0, source });

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d != dist[u]) {
            continue;
        }
        for (const auto& edge : graph[u]) {
            std::uint64_t nd = d + edge.weight;
            if (nd < dist[edge.to]) {
                dist[edge.to] = nd;
                pq.push({ nd, edge.to });
            }
        }
    }

    return dist;
}

// Radix heap implementation for 64-bit unsigned keys.
class RadixHeap {
public:
    RadixHeap() : buckets(65), last(0), sz(0) {}

    bool empty() const { return sz == 0; }
    std::size_t size() const { return sz; }

    void push(std::uint64_t key, int value) {
        std::size_t idx = bucket_index(key ^ last);
        buckets[idx].emplace_back(key, value);
        ++sz;
    }

    std::pair<std::uint64_t, int> pop() {
        if (buckets[0].empty()) {
            relocate();
        }
        auto res = buckets[0].back();
        buckets[0].pop_back();
        --sz;
        return res;
    }

private:
    std::vector<std::vector<std::pair<std::uint64_t, int>>> buckets;
    std::uint64_t last;
    std::size_t sz;

    static std::size_t bucket_index(std::uint64_t diff) {
        if (diff == 0) {
            return 0;
        }
        return 64 - static_cast<std::size_t>(__builtin_clzll(diff));
    }

    void relocate() {
        std::size_t i = 1;
        while (i < buckets.size() && buckets[i].empty()) {
            ++i;
        }
        if (i == buckets.size()) {
            throw std::logic_error("RadixHeap is empty");
        }

        auto new_last = buckets[i][0].first;
        for (const auto& item : buckets[i]) {
            if (item.first < new_last) {
                new_last = item.first;
            }
        }
        last = new_last;

        for (const auto& item : buckets[i]) {
            std::size_t idx = bucket_index(item.first ^ last);
            buckets[idx].push_back(item);
        }
        buckets[i].clear();
    }
};

std::vector<std::uint64_t> breaking_sorting_barrier_sssp(const Graph& graph, int source) {
    const std::uint64_t INF = std::numeric_limits<std::uint64_t>::max();
    std::vector<std::uint64_t> dist(graph.size(), INF);
    dist[source] = 0;

    RadixHeap pq;
    pq.push(0, source);

    while (!pq.empty()) {
        auto [d, u] = pq.pop();
        if (d != dist[u]) {
            continue;
        }
        for (const auto& edge : graph[u]) {
            std::uint64_t nd = d + edge.weight;
            if (nd < dist[edge.to]) {
                dist[edge.to] = nd;
                pq.push(nd, edge.to);
            }
        }
    }

    return dist;
}

struct RunResult {
    std::vector<std::uint64_t> distances;
    std::chrono::duration<double, std::milli> elapsed_ms{};
};

RunResult time_algorithm(const Graph& graph, int source, const std::string& name,
    std::vector<std::uint64_t>(*fn)(const Graph&, int),
    int runs) {
    std::vector<double> samples_ms;
    samples_ms.reserve(static_cast<std::size_t>(runs));
    std::vector<std::uint64_t> dist;

    for (int i = 0; i < runs; ++i) {
        auto start = std::chrono::steady_clock::now();
        auto current = fn(graph, source);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        samples_ms.push_back(elapsed.count());
        if (i == 0) {
            dist = std::move(current);
        }
    }

    auto sum_ms = std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0);
    double avg_ms = sum_ms / static_cast<double>(runs);
    double best_ms = *std::min_element(samples_ms.begin(), samples_ms.end());

    std::cout << std::setw(30) << std::left << name << ": avg=" << std::fixed
        << std::setprecision(3) << avg_ms << " ms, best=" << std::setprecision(3)
        << best_ms << " ms over " << runs << " run(s)" << std::endl;

    return { std::move(dist), std::chrono::duration<double, std::milli>(avg_ms) };
}

void verify_results(const std::vector<std::uint64_t>& a,
    const std::vector<std::uint64_t>& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("Result vectors have different sizes");
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            std::ostringstream oss;
            oss << "Mismatch at node " << i << ": " << a[i] << " vs " << b[i];
            throw std::runtime_error(oss.str());
        }
    }
}

void print_help(const std::string& exe) {
    std::cout << "Usage: " << exe << " <input_file> <source_node> [runs]" << std::endl;
    std::cout << "\nInput file format: each line has 'from to weight' (space or tab separated)." << std::endl;
    std::cout << "Nodes are zero-indexed. Lines starting with # are ignored." << std::endl;
    std::cout << "Optional 'runs' allows repeating each algorithm to smooth timings (default: 1)." << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_help(argv[0]);
        return 1;
    }

    const std::string input_path = argv[1];
    const int source = std::stoi(argv[2]);
    const int runs = (argc >= 4) ? std::max(1, std::stoi(argv[3])) : 1;

    try {
        auto loaded = read_graph_from_file(input_path);
        if (loaded.graph.empty()) {
            throw std::runtime_error("Input graph is empty; provide at least one edge.");
        }
        if (source < 0 || source >= loaded.node_count) {
            throw std::runtime_error("Source node is out of range for the graph");
        }

        std::cout << "Loaded graph with " << loaded.node_count << " nodes." << std::endl;
        RunResult dijkstra_result =
            time_algorithm(loaded.graph, source, "Dijkstra (binary heap)", dijkstra, runs);
        RunResult breaking_result = time_algorithm(loaded.graph, source,
            "Breaking Sorting Barrier SSSP",
            breaking_sorting_barrier_sssp, runs);

        verify_results(dijkstra_result.distances, breaking_result.distances);
        std::cout << "Results match for both algorithms." << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}