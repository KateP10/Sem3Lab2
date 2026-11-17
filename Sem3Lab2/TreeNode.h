#pragma once

#include "Traversal.h"

#include <cctype>     //std::isdigit
#include <functional> //std::function
#include <optional>   //std::optional<T> в parse
#include <queue>      //std::queue в levelOrder
#include <stdexcept>  //std::runtime_error, std::invalid_argument
#include <string>     //std::string
#include <utility>    //std::move
#include <vector>     //std::vector<T>

namespace lab1 {

    template <typename T>
    struct TreeNode { //все поля и методы по умолчанию public, BinaryTree обращается к cur->left, cur->right, cur->data напрямую
        //Parser — функция «строка -> T». Нужна в parse. Formatter -> функция «T → строка». Нужна в serialize.
        //std::function — стирание типа - можно передать лямбду, свободную функцию, функтор — все, что подходит по сигнатуре
        using Parser = std::function<T(const std::string&)>;
        using Formatter = std::function<std::string(const T&)>;

        T data;
        TreeNode* left = nullptr;
        TreeNode* right = nullptr;
        //explicit — запрет неявных преобразований: нельзя написать TreeNode<int> n = 5;, только TreeNode<int> n(5);
        explicit TreeNode(const T& value) : data(value) {} //копирование
        explicit TreeNode(T&& value) : data(std::move(value)) {} //перемещение

        TreeNode(const TreeNode&) = delete; //запрет на копирование напрямую, только через clone() (клон детей, а не общих указателей)
        TreeNode& operator=(const TreeNode&) = delete;

        // ---------- Жизненный цикл ----------
        void destroy() {
            //если есть левый — рекурсивно уничтожить его поддерево, потом удалить сам узел левого; то же для правого
            if (left) { left->destroy();  delete left;  left = nullptr; }
            if (right) { right->destroy(); delete right; right = nullptr; }
        }
        //копирование поддерева. Создаем новый узел с копией data, потом рекурсивно клонируем детей и привязываем к копии
        TreeNode* clone() const {
            auto* n = new TreeNode(data);
            if (left)  n->left = left->clone();
            if (right) n->right = right->clone();
            return n;
        }

        // ---------- Структура ----------
        std::size_t size() const {
            return 1 + (left ? left->size() : 0) + (right ? right->size() : 0);
        }

        // ---------- Поиск ----------
        TreeNode* find(const T& value) {
            if (data == value) return this;
            if (left)  if (auto* r = left->find(value))  return r;
            if (right) if (auto* r = right->find(value)) return r;
            return nullptr;
        }
        //const-версия
        const TreeNode* find(const T& value) const {
            if (data == value) return this;
            if (left)  if (auto* r = left->find(value))  return r;
            if (right) if (auto* r = right->find(value)) return r;
            return nullptr;
        }
        //сравнивает структуру и данные двух поддеревьев
        bool equal(const TreeNode* other) const {
            if (!other) return false;
            if (!(data == other->data)) return false;
            //двойное отрицание превращает указатель в bool: nullptr -> false, иначе true, чтобы сравнить наличие двух указателей, а не их адреса
            if (!!left != !!other->left) return false;
            if (!!right != !!other->right) return false;
            if (left && !left->equal(other->left))  return false; //если левые есть, рекурсивно сравнить
            if (right && !right->equal(other->right)) return false;
            return true;
        }
        //поиск поддерева other внутри текущего дерева
        bool containsSubtree(const TreeNode* other) const {
            if (!other) return true; //пустое искомое есть везде
            if (equal(other)) return true;
            if (left && left->containsSubtree(other))  return true;
            if (right && right->containsSubtree(other)) return true;
            return false;
        }

        // ---------- Обходы ----------
        //Root — добавить значение текущего узла в out; Left — если есть левый, рекурсивно обойти; Right — если есть правый, рекурсивно обойти
        void traverse(const Traversal& t, std::vector<T>& out) const {
            for (Symbol s : t.order) {
                switch (s) {
                case Symbol::Root:  out.push_back(data); break; //out — параметр по ссылке, а не возврат
                case Symbol::Left:  if (left)  left->traverse(t, out);  break;
                case Symbol::Right: if (right) right->traverse(t, out); break;
                }
            }
        }
        //BFS (обход в ширину)
        void levelOrder(std::vector<T>& out) const {
            std::queue<const TreeNode*> q;
            q.push(this);
            while (!q.empty()) {
                const TreeNode* cur = q.front();
                q.pop();
                out.push_back(cur->data);
                if (cur->left)  q.push(cur->left);
                if (cur->right) q.push(cur->right);
            }
        }

        // ---------- Путь ----------
        //проход по пути из L/R. cur — курсор
        const TreeNode* stepByPath(const std::string& path) const {
            const TreeNode* cur = this;
            for (char c : path) {
                if (c == 'L' || c == 'l') {
                    if (!cur->left) throw std::runtime_error("no left child in path");
                    cur = cur->left;
                }
                else if (c == 'R' || c == 'r') {
                    if (!cur->right) throw std::runtime_error("no right child in path");
                    cur = cur->right;
                }
                else {
                    throw std::invalid_argument("path must contain only L/R");
                }
            }
            return cur;
        }

        // ---------- Функциональные ----------
        template <typename F>
        //преобразование всех значений через f
        TreeNode* map(F f) const {
            auto* n = new TreeNode(f(data)); //применяем f к значению текущего узла
            if (left)  n->left = left->map(f);
            if (right) n->right = right->map(f);
            return n;
        }

        template <typename P>
        //фильтрация
        //если предикат pred(data) вернул false — возвращаем nullptr, то есть отсекаем все поддерево
        TreeNode* where(P pred) const {
            if (!pred(data)) return nullptr;
            auto* n = new TreeNode(data);
            if (left)  n->left = left->where(pred);
            if (right) n->right = right->where(pred);
            return n;
        }

        // ---------- Сериализация ----------
        void serialize(const Traversal& t, const Formatter& fmt, std::string& out) const {
            for (Symbol s : t.order) {
                switch (s) {
                case Symbol::Root: { //пишем значение fmt(data) в формате [len:value]
                    const std::string v = fmt(data);
                    out += '[';
                    out += std::to_string(v.size());
                    out += ':';
                    out += v;
                    out += ']';
                    break;
                }
                case Symbol::Left: //пишем (, рекурсивно сериализуем ребенка (или #, если его нет), пишем )
                    out += '(';
                    if (left) left->serialize(t, fmt, out);
                    else      out += '#';
                    out += ')';
                    break;
                case Symbol::Right:
                    out += '(';
                    if (right) right->serialize(t, fmt, out);
                    else       out += '#';
                    out += ')';
                    break;
                }
            }
        }

        // ---------- Парсинг ----------
        //static — потому что парсинг создает новый узел, а не работает с существующим
        static TreeNode* parse(const std::string& s, std::size_t& pos, const Traversal& t, const Parser& parser) {
            if (pos >= s.size()) throw std::invalid_argument("unexpected end of string");
            if (s[pos] == '#') { ++pos; return nullptr; } //# — пустой узел, сдвигаем курсор и возвращаем nullptr

            std::optional<T> value; //значение типа T или ничего
            //нужен, потому что T может не иметь конструктора по умолчанию (например, Person без id и name бессмыслен)
            TreeNode* left = nullptr;
            TreeNode* right = nullptr;

            for (Symbol sym : t.order) {
                switch (sym) {
                    //Ожидаем [; Читаем цифры длины, пока они идут; Ожидаем:; Берем ровно len байт; Ожидаем ]; Разбираем raw через parser и кладем в value
                case Symbol::Root: {
                    if (pos >= s.size() || s[pos] != '[') throw std::invalid_argument("expected '['");
                    ++pos;
                    const std::size_t start = pos;
                    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
                    if (start == pos || pos >= s.size() || s[pos] != ':') throw std::invalid_argument("bad value length");
                    const std::size_t len = std::stoul(s.substr(start, pos - start)); //std::stoul - бросает std::invalid_argument, если строка не число
                    ++pos;
                    if (pos + len > s.size()) throw std::invalid_argument("value too long");
                    const std::string raw = s.substr(pos, len);
                    pos += len;
                    if (pos >= s.size() || s[pos] != ']') throw std::invalid_argument("expected ']'");
                    ++pos;
                    value = parser(raw);
                    break;
                }
                case Symbol::Left: {
                    if (pos >= s.size() || s[pos] != '(') throw std::invalid_argument("expected '('");
                    ++pos;
                    left = parse(s, pos, t, parser);
                    if (pos >= s.size() || s[pos] != ')') throw std::invalid_argument("expected ')'");
                    ++pos;
                    break;
                }
                case Symbol::Right: {
                    if (pos >= s.size() || s[pos] != '(') throw std::invalid_argument("expected '('");
                    ++pos;
                    right = parse(s, pos, t, parser);
                    if (pos >= s.size() || s[pos] != ')') throw std::invalid_argument("expected ')'");
                    ++pos;
                    break;
                }
                }
            }

            if (!value) throw std::invalid_argument("traversal did not include root symbol");

            auto* node = new TreeNode(std::move(*value));
            node->left = left;
            node->right = right;
            return node;
        }
        //построение полного дерева из массива: элемент i -> дети 2i+1 и 2i+2
        static TreeNode* buildComplete(const std::vector<T>& values, std::size_t i) {
            if (i >= values.size()) return nullptr;
            auto* n = new TreeNode(values[i]);
            n->left = buildComplete(values, 2 * i + 1);
            n->right = buildComplete(values, 2 * i + 2);
            return n;
        }
    };

} // namespace lab1