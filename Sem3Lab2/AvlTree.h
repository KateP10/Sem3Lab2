#pragma once

#include "TreeNode.h"   //lab1::TreeNode

#include <algorithm>   //std::max в fix()
#include <cstddef>     //std::size_t
#include <functional>  //std::less, std::move
#include <utility>     //std::pair, std::move
#include <vector>      //std::vector для inOrder/range

namespace lab2 {
    //один «слот» в узле дерева
    template <typename K, typename V>
    struct BstEntry {
        K key{}; //ключ поиска
        V value{}; //полезная нагрузка
        int height = 1; //высота этого узла (нужна для балансировки)
    };
    //K — тип ключа; V — тип значения; 
    //Compare — компаратор, по умолчанию std::less<K>. Для std::string и int работает из коробки
    template <typename K, typename V, typename Compare = std::less<K>>
    class AvlTree {
    public:
        //псевдонимы
        using Entry = BstEntry<K, V>;
        using Node = lab1::TreeNode<Entry>; //Node — это TreeNode из ЛР-1

        AvlTree() = default;
        //копирование запрещено. Дерево владеет узлами через сырые указатели. Обычный copy скопировал бы root_ — и два объекта уничтожили бы одно дерево
        AvlTree(const AvlTree&) = delete;
        AvlTree& operator=(const AvlTree&) = delete;
        //Move-конструктор, при noexcept используется move, а не копирование
        AvlTree(AvlTree&& o) noexcept : root_(o.root_), size_(o.size_), cmp_(std::move(o.cmp_)) {
            o.root_ = nullptr;
            o.size_ = 0;
        }
        //Move-присваивание
        AvlTree& operator=(AvlTree&& o) noexcept {
            if (this != &o) {
                destroyAll(); root_ = o.root_; size_ = o.size_;
                cmp_ = std::move(o.cmp_); o.root_ = nullptr; o.size_ = 0;
            }
            return *this;
        }
        ~AvlTree() { destroyAll(); } //destroyAll рекурсивно уничтожит поддерево (через TreeNode::destroy) и удалит корень

        void clear() { destroyAll(); } //публичный «сброс»
        //вставка с map-семантикой (ключ уникален)
        //insertImpl — рекурсивная вставка, возвращает новый корень поддерева (после возможных вращений)
        //флаг ins — действительно ли вставлен новый ключ
        //если ключ уже был, значение перезаписывается, size_ не меняется. root_ переприсваивается, потому что после ротаций корень поддерева мог смениться
        void insert(const K& k, const V& v) {
            bool ins = false;
            root_ = insertImpl(root_, k, v, ins);
            if (ins) ++size_;
        }

        //вставка с multimap-семантикой: при совпадении ключа v добавляется в value (который должен быть контейнером с push_back)
        template <typename U>
        void insertOrAppend(const K& k, const U& v) {
            bool ins = false;
            root_ = appendImpl(root_, k, v, ins);
            if (ins) ++size_;
        }
        //BST-поиск
        const V* find(const K& k) const {
            const Node* n = root_;
            while (n) {
                if (cmp_(k, n->data.key)) n = n->left; //// k < key -> налево
                else if (cmp_(n->data.key, k)) n = n->right;
                else return &n->data.value;
            }
            return nullptr;
        }
        bool contains(const K& k) const { return find(k) != nullptr; }
        //удаление. eraseImpl вернет новый корень (после балансировки и замены удаленного узла его in-order-преемником)
        //er — действительно ли что-то удалили
        void erase(const K& k) {
            bool er = false;
            root_ = eraseImpl(root_, k, er);
            if (er) --size_;
        }
        //геттеры
        std::size_t size() const { return size_; }
        bool empty() const { return root_ == nullptr; }
        int  height() const { return h(root_); }
        //In-order обход возвращает сортированный по ключу вектор. Нужен для индексов - вывести все по алфавиту
        std::vector<std::pair<K, V>> inOrder() const {
            std::vector<std::pair<K, V>> out;
            inOrderImpl(root_, out);
            return out;
        }
        //диапазонный поиск, возвращает все пары с ключом в [lo, hi]
        std::vector<std::pair<K, V>> range(const K& lo, const K& hi) const {
            std::vector<std::pair<K, V>> out;
            rangeImpl(root_, lo, hi, out);
            return out;
        }

    private:
        //корень, счетчик узлов, компаратор
        Node* root_ = nullptr;
        std::size_t size_ = 0;
        Compare cmp_{};

        static int h(const Node* n) { return n ? n->data.height : 0; } //высота поддерева
        //пересчет высоты узла из высот детей. Вызывается после любой модификации детей (вставки/удаления/поворота)
        static void fix(Node* n) { n->data.height = 1 + std::max(h(n->left), h(n->right)); }
        //положительный -> правое выше, отрицательный -> левое выше
        static int balance(const Node* n) { return h(n->right) - h(n->left); }
        //левый поворот. Было: x с правым ребенком y. Стало: y — корень, x — его левый ребенок, y->left (бывший) стал x->right
        static Node* rotL(Node* x) {
            Node* y = x->right;
            x->right = y->left; 
            y->left = x;
            fix(x); 
            fix(y);
            return y;
        }
        //симметричный правый поворот
        static Node* rotR(Node* y) {
            Node* x = y->left;
            y->left = x->right;
            x->right = y;
            fix(y);
            fix(x); 
            return x;
        }
        static Node* rebalance(Node* n) { //вызывается после рекурсивного спуска
            fix(n); //обновить высоту n (дети уже обновлены рекурсией)
            int b = balance(n);
            if (b > 1) { 
                if (balance(n->right) < 0)  
                    n->right = rotR(n->right); //случай RL 
                return rotL(n); //случай RR
            }
            if (b < -1) { 
                if (balance(n->left) > 0) //LR
                    n->left = rotL(n->left);  
                return rotR(n);  //LL
            }
            return n; //сбалансировано
        }
        //вставка
        Node* insertImpl(Node* n, const K& k, const V& v, bool& ins) {
            if (!n) { //дошли до пустого места, создаем новый лист, height = 1, флаг ins = true
                ins = true; 
                return new Node(Entry{ k, v, 1 }); 
            }
            //k < key -> влево, key < k -> вправо, иначе ключ совпал -> перезапись значения, ins = false
            if (cmp_(k, n->data.key))       n->left = insertImpl(n->left, k, v, ins);
            else if (cmp_(n->data.key, k))  n->right = insertImpl(n->right, k, v, ins);
            else { 
                n->data.value = v; 
                ins = false; 
                return n; 
            }
            return rebalance(n); //rebalance вызывается для каждого узла на пути от вставленного листа до корня
        }
        //Multimap-вставка
        //то же, только при создании нового узла v кладётся в пустой контейнер через push_back
        template <typename U>
        Node* appendImpl(Node* n, const K& k, const U& v, bool& ins) {
            if (!n) {
                ins = true; 
                Entry e; //default-construct
                e.key = k;
                e.value.push_back(v);
                e.height = 1;
                return new Node(std::move(e));
            }
            //при совпадении ключа — добавление в существующий контейнер, size_ не увеличивается (новый ключ не появился)
            if (cmp_(k, n->data.key))       n->left = appendImpl(n->left, k, v, ins);
            else if (cmp_(n->data.key, k))  n->right = appendImpl(n->right, k, v, ins);
            else { 
                n->data.value.push_back(v); 
                ins = false;
                return n; 
            }
            return rebalance(n);
        }
        //классический in-order: сначала левое (все ключи меньше), потом узел, потом правое (все ключи больше). Результат — отсортированный вектор
        static void inOrderImpl(const Node* n, std::vector<std::pair<K, V>>& out) {
            if (!n) return;
            inOrderImpl(n->left, out);
            out.emplace_back(n->data.key, n->data.value);
            inOrderImpl(n->right, out);
        }
        //отсечение
        //key < lo -> весь узел и его левое поддерево вне диапазона (там ключи еще меньше), идем только вправо;
        //key > hi -> симметрично, идем только влево;
        //lo <= key <= hi -> узел в диапазоне, добавляем, и оба поддерева могут содержать еще попадания
        void rangeImpl(const Node* n, const K& lo, const K& hi, std::vector<std::pair<K, V>>& out) const {
            if (!n) return;
            if (cmp_(n->data.key, lo)) { rangeImpl(n->right, lo, hi, out); return; }
            if (cmp_(hi, n->data.key)) { rangeImpl(n->left, lo, hi, out); return; }
            rangeImpl(n->left, lo, hi, out);
            out.emplace_back(n->data.key, n->data.value);
            rangeImpl(n->right, lo, hi, out);
        }
        //найти минимальный узел в поддереве (самый левый), вернуть его через out, а из дерева его удалить, вернув новый корень
        //возвращаем n->right вместо n — у минимального узла нет левого ребенка (он минимальный), но может быть правый, который нужно сохранить
        static Node* extractMin(Node* n, Node*& out) {
            if (!n->left) { 
                out = n;
                return n->right;
            }
            n->left = extractMin(n->left, out);
            return rebalance(n);
        }
        //удаление
        Node* eraseImpl(Node* n, const K& k, bool& er) {
            if (!n) return nullptr;
            if (cmp_(k, n->data.key))       n->left = eraseImpl(n->left, k, er);
            else if (cmp_(n->data.key, k))  n->right = eraseImpl(n->right, k, er);
            else { //нашли узел для удаления. Если нет правого ребенка, то левое поддерево (или nullptr) встает на место n
                er = true;
                Node* L = n->left; 
                Node* R = n->right;
                if (!R) { 
                    n->left = n->right = nullptr;
                    delete n; 
                    return L;
                }
                //если есть правый ребенок — находим in-order-преемника (минимальный в правом поддереве), он встает на место n
                Node* succ = nullptr;
                R = extractMin(R, succ);
                succ->left = L; 
                succ->right = R;
                n->left = n->right = nullptr;
                delete n;
                return rebalance(succ);
            }
            return rebalance(n);
        }
        //TreeNode::destroy() из ЛР-1: рекурсивно удаляет детей и обнуляет указатели
        void destroyAll() {
            if (root_) { root_->destroy(); delete root_; root_ = nullptr; }
            size_ = 0;
        }
    };

} // namespace lab2