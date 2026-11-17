#pragma once

#include "AvlTree.h"
#include "Person.h"

#include <cmath>       //std::sqrt, std::floor
#include <functional>  //std::function
#include <stdexcept>   //std::invalid_argument
#include <utility>     //std::move, std::pair
#include <vector>      //std::vector

//вход: последовательность элементов (например, объектов Person) + параметры гистограммы (диапазон значений, разбиение)
//выход: IDictionary / IMap, где ключ — диапазон(элементарное множество), значение — количество элементов в этом диапазоне(или более богатое описание класса)
//нужно разбить числовую ось [lo, hi] на n равных отрезков и посчитать, сколько объектов попадает в каждый
//плюс — хранить статистику по каждому бакету (среднее, разброс, min/max)
//MAP: пробегаем исходную коллекцию, каждому элементу присваиваем номер бакета -> получаем последовательность (bucketIdx, value)
//REDUCE: свёртываем эту последовательность по ключу bucketIdx, накапливая Bucket в ассоциативном контейнере

namespace lab2 {

    struct Bucket {
        double lo = 0, hi = 0; //границы бакета
        std::size_t count = 0; //число значений
        double sumAge = 0, sumSqAge = 0; //сумма значений и сумма квадратов
        int minAge = 0, maxAge = 0; //минимум и максимум
        bool hasValues = false; //флаг наличия данных

        double meanAge() const { return count ? sumAge / count : 0.0; } //среднее
        double varianceAge() const { //дисперсия E[v2] − E[v]2
            if (count < 2) return 0.0;
            double m = meanAge();
            return sumSqAge / count - m * m;
        }
        double stdDevAge() const { //стандартное отклонение
            double v = varianceAge();
            return v > 0.0 ? std::sqrt(v) : 0.0;
        }
        void add(double v) { //инкрементальное обновление агрегатов
            ++count;
            sumAge += v; 
            sumSqAge += v * v;
            int iv = static_cast<int>(v);
            if (!hasValues) { 
                minAge = maxAge = iv; 
                hasValues = true;
            }
            else { 
                if (iv < minAge)
                    minAge = iv; 
                if (iv > maxAge)
                    maxAge = iv; 
            }
        }
        void merge(const Bucket& o) { //слияние двух бакетов. Все аддитивные поля — просто складываются
            count += o.count; 
            sumAge += o.sumAge; 
            sumSqAge += o.sumSqAge;
            if (o.hasValues) {
                if (!hasValues) { 
                    minAge = o.minAge; 
                    maxAge = o.maxAge; 
                    hasValues = true;
                }
                else {
                    if (o.minAge < minAge) minAge = o.minAge;
                    if (o.maxAge > maxAge) maxAge = o.maxAge;
                }
            }
        }
    };

    class Histogram {
    public:
        using Criterion = std::function<double(const Person&)>; //любой критерий
        //lo, hi — диапазон оси, n — число бакетов, c — критерий
        Histogram(double lo, double hi, std::size_t n, Criterion c) : lo_(lo), hi_(hi), n_(n), criterion_(std::move(c)) {
            if (n_ == 0) throw std::invalid_argument("bucketsCount > 0");
            if (hi_ <= lo_) throw std::invalid_argument("hi > lo");
        }

        void add(const Person& p) {
            double v = criterion_(p); //значение критерия
            //bucketIndex возвращает -1 для значений вне [lo_, hi_]. Такие игнорируем
            int idx = bucketIndex(v);
            if (idx < 0) return;
            Bucket b;
            //find возвращает const Bucket*. Чтобы обновить, нужно скопировать Bucket, обновить копию, вставить обратно
            if (const auto* e = buckets_.find(idx)) b = *e;
            else { 
                b.lo = lo_ + idx * step(); 
                b.hi = b.lo + step();
            }
            b.add(v);
            buckets_.insert(idx, b); //кладем обновленный бакет обратно
        }
        void build(const std::vector<Person>& ps) { //полная перестройка
            buckets_ = {};
            for (const auto& p : ps) add(p);
        }
        //возвращает все бакеты в порядке номеров
        std::vector<Bucket> all() const {
            std::vector<Bucket> out;
            for (auto& pr : buckets_.inOrder()) out.push_back(pr.second);
            return out;
        }
        //бакет, содержащий значение v. nullptr — если v вне диапазона или бакет пуст
        const Bucket* bucketAt(double v) const {
            int idx = bucketIndex(v);
            return idx < 0 ? nullptr : buckets_.find(idx);
        }
        //сырой доступ к внутреннему представлению: пары (индекс, Bucket)
        std::vector<std::pair<int, Bucket>> raw() const { return buckets_.inOrder(); }
        //геттеры и step(). step — ширина одного бакета
        double rangeLo() const { return lo_; }
        double rangeHi() const { return hi_; }
        std::size_t bucketsCount() const { return n_; }
        double step() const { return (hi_ - lo_) / n_; }

        //map-reduce
        static Histogram mapReduce(const std::vector<Person>& ps, double lo, double hi, std::size_t n, Criterion c){
            if (n == 0) throw std::invalid_argument("bucketsCount > 0");
            double s = (hi - lo) / n;

            //MAP
            //локальная структура для пары (bucketIdx, value). reserve — чтобы не было переаллокаций
            struct M { int idx; double v; };
            std::vector<M> mapped; mapped.reserve(ps.size());
            //вычисляем значение критерия v; пропускаем v вне[lo, hi]; вычисляем номер бакета : floor((v - lo) / s);
            //защита от v == hi: если v == hi, то(hi - lo) / s = n, и floor даст n, что за пределами валидных индексов 0..n - 1
            //поэтому клампим i = n - 1, складываем в mapped
            for (const auto& p : ps) {
                double v = c(p);
                if (v < lo || v > hi) continue;
                int i = static_cast<int>(std::floor((v - lo) / s));
                if (i >= static_cast<int>(n)) i = static_cast<int>(n) - 1;
                mapped.push_back({ i, v });
            }
            //REDUCE
            //свертываем пары по ключу bucketIdx, накапливая Bucket
            //логика та же, что в add(), но работает с уже вычисленными индексами
            AvlTree<int, Bucket> acc;
            for (const auto& m : mapped) {
                Bucket b;
                if (const auto* e = acc.find(m.idx)) b = *e;
                else { 
                    b.lo = lo + m.idx * s; 
                    b.hi = b.lo + s;
                }
                b.add(m.v);
                acc.insert(m.idx, b);
            }
            Histogram h(lo, hi, n, c);
            h.buckets_ = std::move(acc);
            return h;
        }

    private:
        double lo_, hi_; //диапазон
        std::size_t n_; //число бакетов
        Criterion criterion_; //критерий
        AvlTree<int, Bucket> buckets_; //хранилище
        //-1 — вне диапазона; floor — чтобы 0.99 попал в бакет 0 (а не в 1);
        int bucketIndex(double v) const {
            if (v < lo_ || v > hi_) return -1;
            int i = static_cast<int>(std::floor((v - lo_) / step()));
            if (i >= static_cast<int>(n_)) i = static_cast<int>(n_) - 1;
            return i;
        }
    };

} // namespace lab2