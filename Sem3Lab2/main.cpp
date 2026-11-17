#include "AvlTree.h"
#include "Person.h"
#include "SegmentTree.h"
#include "Index.h"
#include "Histogram.h"
#include "Benchmark.h"
#include "Csv.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace lab2;

void runAllTests();

static std::vector<Person> makeRandomPersons(std::size_t n, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> age(18, 80);
    std::uniform_int_distribution<int> id(1, 1000000);
    std::uniform_int_distribution<int> nm(0, 5);
    static const char* F[] = { "Ivan","Petr","Anna","Olga","Sergey","Maria" };
    static const char* L[] = { "Ivanov","Petrov","Sidorova","Smirnova","Kuznetsov","Popova" };
    std::vector<Person> ps; ps.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        ps.emplace_back(id(rng), F[nm(rng)], "X", L[nm(rng)], 2024 - age(rng));
    return ps;
}

static std::string personLine(const Person& p) {
    std::ostringstream os;
    os << '[' << p.GetId() << "] " << p.GetFullName()
        << " (b." << p.GetBirthYear() << ", age " << p.GetAge(2024) << ')';
    return os.str();
}

static void demoAvl() {
    std::cout << "\n--- AVL demo ---\n";
    AvlTree<int, std::string> t;
    for (int i = 0; i < 10; ++i) t.insert(i, "v" + std::to_string(i));
    std::cout << "size=" << t.size() << " height=" << t.height() << "\n";
    std::cout << "in-order: ";
    for (auto& pr : t.inOrder()) std::cout << pr.first << ' ';
    std::cout << "\nrange[3..6]: ";
    for (auto& pr : t.range(3, 6)) std::cout << pr.first << ' ';
    std::cout << '\n';
}

static void demoSegmentTree() {
    std::cout << "\n--- Segment tree demo ---\n";
    std::vector<long long> a = { 5,3,8,1,9,2,7,4 };
    SegmentTree<long long, std::plus<long long>> st(a, std::plus<long long>{}, 0);
    std::cout << "array: "; for (auto v : a) std::cout << v << ' '; std::cout << '\n';
    std::cout << "sum[0,7]=" << st.query(0, 7)
        << " sum[2,5]=" << st.query(2, 5) << '\n';
    st.update(3, 100);
    std::cout << "after update(3,100): sum[2,5]=" << st.query(2, 5) << '\n';
}

static void demoIndex() {
    std::cout << "\n--- Indexing demo ---\n";
    auto ps = makeRandomPersons(20);
    Index idx; idx.build(ps);
    for (const auto& p : ps) std::cout << personLine(p) << '\n';

    std::string name;
    std::cout << "First name to search (empty = random): ";
    std::getline(std::cin, name);
    if (name.empty()) name = ps.front().GetFirstName();
    auto found = idx.findByFirstName(name);
    std::cout << "Found " << found.size() << " persons named \"" << name << "\":\n";
    for (auto* p : found) std::cout << "  " << personLine(*p) << '\n';

    std::cout << "Range birthYear [1990..2000]:\n";
    for (auto* p : idx.findByBirthYearRange(1990, 2000))
        std::cout << "  " << personLine(*p) << '\n';
}

static void demoHistogram() {
    std::cout << "\n--- Histogram demo ---\n";
    auto ps = makeRandomPersons(50);
    auto crit = [](const Person& p) { return static_cast<double>(p.GetAge(2024)); };
    Histogram h(18, 80, 6, crit);
    h.build(ps);
    for (const auto& b : h.all())
        std::cout << '[' << b.lo << ".." << b.hi << "): n=" << b.count
        << " mean=" << b.meanAge() << " std=" << b.stdDevAge() << '\n';
}

static std::vector<std::vector<std::string>> g_csv;

static void benchIndex() {
    g_csv.clear();
    g_csv.push_back({ "n", "naive_ms", "avl_ms" });

    std::vector<int> ns = { 1000, 5000, 10000, 50000, 100000 };
    for (int n : ns) {
        auto ps = makeRandomPersons(n);
        Index idx;
        double tBuild = timeMs([&] { idx.build(ps); });

        std::mt19937 rng(42);
        std::uniform_int_distribution<std::size_t> dist(0, ps.size() - 1);
        std::vector<int> q(500);
        for (auto& x : q) x = ps[dist(rng)].GetBirthYear();

        double tNaive = timeMs([&] {
            volatile std::size_t sink = 0;
            for (int y : q)
                for (const auto& p : ps)
                    if (p.GetBirthYear() == y) ++sink;
            (void)sink;
            });
        double tAvl = timeMs([&] {
            volatile std::size_t sink = 0;
            for (int y : q) sink += idx.findByBirthYear(y).size();
            (void)sink;
            });
        std::cout << "n=" << n << ": build=" << tBuild
            << " ms, naive=" << tNaive << " ms, avl=" << tAvl << " ms\n";
        g_csv.push_back({ std::to_string(n), std::to_string(tNaive), std::to_string(tAvl) });
    }
}

static void benchSegment() {
    g_csv.clear();
    g_csv.push_back({ "n", "naive_ms", "seg_ms" });
    std::vector<int> ns = { 1000, 10000, 100000, 1000000 };
    for (int n : ns) {
        std::vector<long long> a(n);
        std::mt19937 rng(1);
        for (auto& v : a) v = rng() % 1000;
        SegmentTree<long long, std::plus<long long>> st(a, std::plus<long long>{}, 0);

        const int Q = 1000;
        std::vector<std::pair<std::size_t, std::size_t>> qs;
        qs.reserve(Q);
        for (int i = 0; i < Q; ++i) {
            std::size_t l = rng() % n, r = rng() % n;
            if (l > r) std::swap(l, r);
            qs.push_back({ l, r });
        }
        double tNaive = timeMs([&] {
            volatile long long sink = 0;
            for (auto& p : qs) {
                long long s = 0;
                for (std::size_t i = p.first; i <= p.second; ++i) s += a[i];
                sink += s;
            }
            (void)sink;
            });
        double tSeg = timeMs([&] {
            volatile long long sink = 0;
            for (auto& p : qs) sink += st.query(p.first, p.second);
            (void)sink;
            });
        std::cout << "n=" << n << ": naive=" << tNaive
            << " ms, seg=" << tSeg << " ms\n";
        g_csv.push_back({ std::to_string(n), std::to_string(tNaive), std::to_string(tSeg) });
    }
}

int main() {
    for (;;) {
        std::cout << "\n=== Lab 2 ===\n"
            "1. AVL demo\n"
            "2. Segment tree demo\n"
            "3. Indexing (I-5)\n"
            "4. Histogram (I-1)\n"
            "5. Benchmark: BST vs naive\n"
            "6. Benchmark: Segment vs naive\n"
            "7. Export last benchmark to CSV\n"
            "8. Run tests\n"
            "0. Exit\n> ";
        int c;
        if (!(std::cin >> c)) break;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        switch (c) {
        case 1: demoAvl(); break;
        case 2: demoSegmentTree(); break;
        case 3: demoIndex(); break;
        case 4: demoHistogram(); break;
        case 5: benchIndex(); break;
        case 6: benchSegment(); break;
        case 7:
            if (g_csv.empty()) { std::cout << "No data.\n"; break; }
            if (saveCsv("benchmark.csv", g_csv))
                std::cout << "Saved benchmark.csv\n";
            else std::cout << "Failed.\n";
            break;
        case 8: runAllTests(); break;
        case 0: return 0;
        default: std::cout << "Unknown.\n";
        }
    }
    return 0;
}