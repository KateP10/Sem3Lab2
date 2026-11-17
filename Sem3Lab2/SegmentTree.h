#pragma once

#include "TreeNode.h"

#include <cstddef>     //std::size_t
#include <stdexcept>   //std::out_of_range
#include <utility>     //std::move
#include <vector>      //std::vector<T>

//дерево отрезков дает O(log n) на запрос и на обновление одного элемента, за счет O(n) памяти и O(n) времени на построение. Идея:
//строим бинарное дерево над индексами массива.Каждый узел отвечает за некоторый отрезок[l, r]
//корень — за весь[0, n - 1]. Листья — за отдельные[i, i]. Внутренний узел[l, r] с m = (l + r) / 2 имеет левого ребенка[l, m] и правого[m + 1, r]
//в каждом узле храним агрегат по своему отрезку: сумму, минимум, что угодно. Агрегат внутреннего узла = merge(агрегат левого, агрегат правого).
//запрос query(l, r) спускается по дереву, но не во все узлы: если узел целиком покрыт запросом — возвращаем его агрегат сразу(это O(1) от поддерева); 
//если пересечения нет — возвращаем нейтральный элемент; если частичное — рекурсивно спускаемся и сливаем результаты.
//умное отсечение дает O(log n + k) узлов, где k — «стыков» отрезков, и в сумме O(log n)
//обновление update(i, v) — спуск от корня по одному пути к листу i, изменение листа, потом пересчет агрегатов на обратном ходе. O(log n)

//каждый узел дерева — это lab1::TreeNode<T>: n->data — агрегат по отрезку(тип T); n->left — левый ребёнок(отрезок[l, m]); n->right — правый([m + 1, r]);

namespace lab2 {
    //T — тип агрегата; Merge — функтор с operator()(const T&, const T&)->T
    template <typename T, typename Merge>
    class SegmentTree {
        using Node = lab1::TreeNode<T>;
    public:
        SegmentTree(const std::vector<T>& values, Merge merge, T neutral) : merge_(std::move(merge)), neutral_(std::move(neutral)), n_(values.size()) {
            if (n_) root_ = build(values, 0, n_ - 1);
        }
        ~SegmentTree() { destroy(); } //destroy() рекурсивно освобождает дерево
        //копирование запрещено
        SegmentTree(const SegmentTree&) = delete;
        SegmentTree& operator=(const SegmentTree&) = delete;

        std::size_t size() const { return n_; } //размер исходного массива
        //запрос агрегата на [l, r] (включительно)
        //qImpl — рекурсивная реализация, [0, n_-1] как текущий отрезок корня
        T query(std::size_t l, std::size_t r) const {
            if (l > r || r >= n_) throw std::out_of_range("SegmentTree::query");
            return qImpl(root_, 0, n_ - 1, l, r);
        }
        //заменить a[i] на v
        void update(std::size_t i, const T& v) {
            if (i >= n_) throw std::out_of_range("SegmentTree::update");
            uImpl(root_, 0, n_ - 1, i, v);
        }
        T get(std::size_t i) const { return query(i, i); } //чтение — через query на одноэлементном отрезке

    private:
        Node* root_ = nullptr; //корень дерева
        Merge merge_; //merge_ — функтор Merge
        T neutral_; //neutral_ — нейтральный элемент. Возвращается из qImpl, когда узел не пересекается с запросом
        std::size_t n_ = 0; //длина массива

        Node* build(const std::vector<T>& a, std::size_t l, std::size_t r) {
            Node* n = new Node(neutral_); //узел с нейтральным значением «по умолчанию»
            if (l == r) { //лист
                n->data = a[l]; 
                return n; 
            }
            std::size_t m = (l + r) / 2; //середина
            //строим левого и правого ребенка. Ровно n листьев и n-1 внутренних узлов — всего 2n - 1 узлов
            n->left = build(a, l, m);
            n->right = build(a, m + 1, r);
            n->data = merge_(n->left->data, n->right->data); //merge агрегатов детей
            return n;
        }
        //запрос
        //[l, r] — отрезок, за который отвечает n; [ql, qr] — искомый отрезок
        T qImpl(const Node* n, std::size_t l, std::size_t r, std::size_t ql, std::size_t qr) const {
            if (qr < l || r < ql) return neutral_; //вне запроса
            if (ql <= l && r <= qr) return n->data; //узел полностью покрыт запросом. [l, r] целиком в [ql, qr]
            //частичное пересечение. Спускаемся в оба ребенка и сливаем результаты через merge
            std::size_t m = (l + r) / 2;
            return merge_(qImpl(n->left, l, m, ql, qr), qImpl(n->right, m + 1, r, ql, qr));
        }
        //обновление значения
        void uImpl(Node* n, std::size_t l, std::size_t r, std::size_t i, const T& v) {
            if (l == r) {
                n->data = v; 
                return;
            }
            std::size_t m = (l + r) / 2;
            if (i <= m) uImpl(n->left, l, m, i, v); //i в левой половине
            else        uImpl(n->right, m + 1, r, i, v);
            n->data = merge_(n->left->data, n->right->data); //пересчитываем агрегаты всех узлов на пути от листа до корня
        }
        void destroy() {
            if (root_) { 
                root_->destroy(); //использует TreeNode::destroy() из ЛР-1: рекурсивно удаляет детей и обнуляет указатели в детях
                delete root_; 
                root_ = nullptr; }
        }
    };

} // namespace lab2