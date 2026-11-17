#include "AvlTree.h"
#include "Person.h"
#include "SegmentTree.h"
#include "Index.h"
#include "Histogram.h"
#include "Benchmark.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace lab2;

static std::vector<Person> makeRandomPersons(std::size_t n, unsigned seed) {
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

static void testAvlBasic() {
    AvlTree<int, std::string> t;
    for (int i = 0; i < 1000; ++i) t.insert(i, "v" + std::to_string(i));
    assert(t.size() == 1000);
    for (int i = 0; i < 1000; ++i) {
        const auto* v = t.find(i);
        assert(v && *v == "v" + std::to_string(i));
    }
    assert(t.find(1001) == nullptr);
    auto all = t.inOrder();
    assert(all.size() == 1000);
    for (std::size_t i = 1; i < all.size(); ++i)
        assert(all[i - 1].first < all[i].first);
    assert(t.height() < 32);
}

static void testAvlRange() {
    AvlTree<int, int> t;
    for (int i = 0; i < 100; ++i) t.insert(i, i * 10);
    auto r = t.range(20, 30);
    assert(r.size() == 11);
    for (const auto& pr : r) {
        assert(pr.first >= 20 && pr.first <= 30);
        assert(pr.second == pr.first * 10);
    }
}

static void testAvlErase() {
    AvlTree<int, int> t;
    for (int i = 0; i < 100; ++i) t.insert(i, i);
    for (int i = 0; i < 100; i += 2) t.erase(i);
    assert(t.size() == 50);
    for (int i = 0; i < 100; ++i)
        assert((i % 2 == 0) ? !t.contains(i) : t.contains(i));
}

static void testSegmentTree() {
    std::vector<long long> a = { 1,2,3,4,5,6,7,8,9,10 };
    SegmentTree<long long, std::plus<long long>> st(a, std::plus<long long>{}, 0);
    assert(st.query(0, 9) == 55);
    assert(st.query(2, 5) == 18);
    st.update(4, 50);
    assert(st.get(4) == 50);
    assert(st.query(2, 5) == 63);
    assert(st.query(0, 9) == 100);
}

static void testIndex() {
    auto ps = makeRandomPersons(500, 42);
    Index idx; idx.build(ps);

    for (const auto& p : ps) {
        auto found = idx.findByFirstName(p.GetFirstName());
        bool ok = false;
        for (auto* q : found) if (q->GetId() == p.GetId()) { ok = true; break; }
        assert(ok);
    }

    auto rng = idx.findByBirthYearRange(2000, 2006);
    std::size_t lin = 0;
    for (const auto& p : ps)
        if (p.GetBirthYear() >= 2000 && p.GetBirthYear() <= 2006) ++lin;
    assert(rng.size() == lin);

    const auto& p0 = ps[0];
    auto found = idx.findByFirstLastYear(p0.GetFirstName(), p0.GetLastName(), p0.GetBirthYear());
    bool ok = false;
    for (auto* q : found) if (q->GetId() == p0.GetId()) { ok = true; break; }
    assert(ok);

    auto found2 = idx.findByFullNameAge(p0.GetFullName(), p0.GetAge(2019));
    ok = false;
    for (auto* q : found2) if (q->GetId() == p0.GetId()) { ok = true; break; }
    assert(ok);
}

static void testHistogram() {
    auto ps = makeRandomPersons(1000, 7);
    auto crit = [](const Person& p) { return static_cast<double>(p.GetAge(2024)); };
    Histogram h(18, 80, 10, crit);
    h.build(ps);

    std::size_t sum = 0;
    for (const auto& b : h.all()) sum += b.count;
    assert(sum == ps.size());

    for (const auto& b : h.all())
        if (b.count >= 2) {
            assert(b.stdDevAge() >= 0.0);
            assert(b.minAge >= static_cast<int>(b.lo) - 1);
            assert(b.maxAge <= static_cast<int>(b.hi) + 1);
        }

    auto hr = Histogram::mapReduce(ps, 18, 80, 10, crit);
    auto ha = h.all(); auto hb = hr.all();
    assert(ha.size() == hb.size());
    for (std::size_t i = 0; i < ha.size(); ++i) assert(ha[i].count == hb[i].count);
}

void runAllTests() {
    testAvlBasic();
    testAvlRange();
    testAvlErase();
    testSegmentTree();
    testIndex();
    testHistogram();
    std::cout << "All tests passed.\n";
}