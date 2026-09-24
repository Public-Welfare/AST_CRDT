#include "tree_crdt/document.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace tree_crdt;

void run_benchmark(size_t operations = 10000) {
    std::cout << "==================================================" << std::endl;
    std::cout << "   TreeCRDT Performance Benchmark vs Naive String " << std::endl;
    std::cout << "   Workload: " << operations << " Random-Access Insertions   " << std::endl;
    std::cout << "==================================================\n" << std::endl;

    std::mt19937_64 rng(42);

    // 1. Benchmark Naive std::string
    std::string naive_doc;
    naive_doc.reserve(operations * 10);

    auto start_naive = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < operations; ++i) {
        size_t max_pos = naive_doc.size();
        std::uniform_int_distribution<size_t> dist(0, max_pos);
        size_t pos = dist(rng);
        naive_doc.insert(pos, "x");
    }
    auto end_naive = std::chrono::high_resolution_clock::now();
    double naive_ms = std::chrono::duration<double, std::milli>(end_naive - start_naive).count();

    // Reset RNG
    rng.seed(42);

    // 2. Benchmark TreeCRDT
    Document crdt_doc(1);

    auto start_tree = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < operations; ++i) {
        size_t max_pos = crdt_doc.length();
        std::uniform_int_distribution<size_t> dist(0, max_pos);
        size_t pos = dist(rng);
        crdt_doc.insert(pos, "x");
    }
    auto end_tree = std::chrono::high_resolution_clock::now();
    double tree_ms = std::chrono::duration<double, std::milli>(end_tree - start_tree).count();

    double tree_ops_sec = (operations / tree_ms) * 1000.0;
    double naive_ops_sec = (operations / naive_ms) * 1000.0;
    double speedup = naive_ms / std::max(tree_ms, 0.001);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Results:" << std::endl;
    std::cout << "  - Document Final Size:       " << crdt_doc.length() << " chars" << std::endl;
    std::cout << "  - AVL Tree Final Height:     " << crdt_doc.tree_height() << std::endl;
    std::cout << "  - Naive String Time:         " << naive_ms << " ms (" << static_cast<size_t>(naive_ops_sec) << " ops/sec)" << std::endl;
    std::cout << "  - TreeCRDT (C++20) Time:     " << tree_ms << " ms (" << static_cast<size_t>(tree_ops_sec) << " ops/sec)" << std::endl;
    std::cout << "  - Throughput Speedup:        " << speedup << "x faster than naive string\n" << std::endl;
}

int main() {
    run_benchmark(10000);
    return 0;
}
