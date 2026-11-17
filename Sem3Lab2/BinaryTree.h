#pragma once

#include "TreeNode.h"

#include <ostream>     //std::ostream для is_streamable
#include <queue>       //std::queue в insertLevelOrder
#include <sstream>     //std::ostringstream в defaultFormatterImpl
#include <stdexcept>   //std::logic_error, std::invalid_argument, std::runtime_error
#include <string>      //std::string
#include <type_traits> //std::integral_constant, std::is_pointer, std::is_function
#include <utility>     //std::declval, std::move
#include <vector>      //std::vector<T>

namespace lab1 {

    template <typename T>
    class is_streamable {
        template <typename U>
        //есть ли operator<< для T?
        //проверяет, компилируется ли выражение oss << value для T
        //std::declval<std::ostream&>() — воображаемая ссылка на std::ostream
        //не вызывает конструктор, только дает тип для проверки. Внутри decltype не вычисляется
        //std::declval<const U&>() — так же для значения типа T
        //<< — оператор, который мы проверяем
        //все это в decltype(...) — выражение не вычисляется, только проверяется, что оно синтаксически корректно
        //std::true_type{} — второй аргумент, если первая часть валидна, decltype возвращает тип второго выражения, то есть std::true_type
        //-> decltype(...) — trailing return type
        //возвращаемый тип test(int) — либо std::true_type, либо(в случае ошибки SFINAE) компилятор попробует перегрузку ниже
        static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<const U&>(), std::true_type{});

        template <typename U>
        //запасная перегрузка
        //принимает что угодно (... — вариативные аргументы C++)
        //возвращает std::false_type
        static std::false_type test(...);

    public:
        //вызываем test<T>(0) — если operator<< есть, вернется std::true_type, из которого берем ::value = true. Если нет — std::false_type, ::value = false.
        //constexpr — значение доступно на этапе компиляции
        static constexpr bool value = decltype(test<T>(0))::value;
    };

    //это указатель на функцию?
    //std::is_pointer<T>::value — это вообще указатель?
    //std::remove_pointer<T>::type — снимает* : int(*)(int) -> int(int)
    //std::is_function<...>::value — а то, что осталось, это функция ?
    //если оба true — значит T — указатель на функцию
    template <typename T>
    struct is_function_pointer : std::integral_constant<bool, std::is_pointer<T>::value&& std::is_function<typename std::remove_pointer<T>::type>::value> {};

    //можно ли печатать T по умолчанию?
    //is_streamable<T> — есть operator<<;
    //!is_function_pointer<T> — и это не указатель на функцию
    //три метафункции нужны чтобы выбрать, что делать в saveToString, на этапе компиляции, а не в рантайме
    template <typename T>
    struct is_default_printable : std::integral_constant<bool, is_streamable<T>::value && !is_function_pointer<T>::value> {};

    template <typename T>
    class BinaryTree {
    public:
        //Parser и Formatter — это std::function<T(const std::string&)> и std::function<std::string(const T&)>, объявленные в TreeNode (псевдонимы)
        using Parser = typename TreeNode<T>::Parser;
        using Formatter = typename TreeNode<T>::Formatter;

    private:
        TreeNode<T>* root_ = nullptr; //указатель на корень дерева
        //возвращает лямбду, которая через oss << v превращает значение в строку, дефолтный форматтер
        static Formatter defaultFormatterImpl(std::true_type) {
            return [](const T& v) {
                std::ostringstream oss;
                oss << v;
                return oss.str();
                };
        }
        //если Т не печатается - бросает исключение
        static Formatter defaultFormatterImpl(std::false_type) {
            throw std::logic_error("type is not streamable; pass a custom formatter to saveToString");
        }

    public:
        BinaryTree() = default; //дефолтный конструктор, root_ уже инициализирован nullptr в объявлении

        BinaryTree(const BinaryTree& other) : root_(other.root_ ? other.root_->clone() : nullptr) {} //копирование

        BinaryTree(BinaryTree&& other) noexcept : root_(other.root_) { //перемещение
            other.root_ = nullptr;
        }

        BinaryTree& operator=(const BinaryTree& other) {
            if (this != &other) {
                clear();
                root_ = other.root_ ? other.root_->clone() : nullptr;
            }
            return *this;
        }

        BinaryTree& operator=(BinaryTree&& other) noexcept {
            if (this != &other) { //защита от a = a, без нее clear() уничтожил бы собственные данные до копирования
                clear();
                root_ = other.root_;
                other.root_ = nullptr;
            }
            return *this;
        }

        ~BinaryTree() { clear(); }

        void clear() { //освоюодить все
            if (root_) {
                root_->destroy(); //рекурсивно освободить детей
                delete root_;
                root_ = nullptr;
            }
        }

        bool empty() const { return root_ == nullptr; }
        std::size_t size() const { return root_ ? root_->size() : 0; } //root_->size() рекурсивно считает узлы
        //пересобирает дерево из массива по правилу «элемент i -> дети 2i+1 и 2i+2». Сначала clear() — чтобы не смешивать старое дерево с новым
        void buildFromLevelOrder(const std::vector<T>& values) {
            clear();
            root_ = TreeNode<T>::buildComplete(values, 0); //buildComplete — статический
        }
        //добавляет значение в первое свободное место при обходе в ширину
        //алгоритм:
        //если дерево пустое — корень = новый узел
        //для каждого узла:
        //если левого нет — создать там, вернуть;
        //если правого нет — создать там, вернуть;
        //иначе обоих детей в очередь, продолжить
        void insertLevelOrder(const T& value) {
            if (!root_) {
                root_ = new TreeNode<T>(value);
                return;
            }
            std::queue<TreeNode<T>*> q;
            q.push(root_);
            while (!q.empty()) {
                TreeNode<T>* cur = q.front();
                q.pop();
                if (!cur->left) {
                    cur->left = new TreeNode<T>(value);
                    return;
                }
                q.push(cur->left);
                if (!cur->right) {
                    cur->right = new TreeNode<T>(value);
                    return;
                }
                q.push(cur->right);
            }
        }
        //обертка над TreeNode::traverse. Создает пустой вектор, отдает его root_->traverse (по ссылке), возвращает заполненный
        std::vector<T> traversal(const Traversal& t) const {
            std::vector<T> out;
            if (root_) root_->traverse(t, out);
            return out;
        }
        //если форматтер передан — используем его
        //иначе — выбираем дефолтный через defaultFormatterImpl с нужным integral_constant:
        //is_default_printable<T>::value == true -> integral_constant<bool, true> -> перегрузка с лямбдой; false -> перегрузка с throw
        std::string saveToString(const Traversal& t, Formatter fmt = nullptr) const {
            if (!fmt) {
                fmt = defaultFormatterImpl(std::integral_constant<bool, is_default_printable<T>::value>{});
            }
            std::string out;
            if (root_) root_->serialize(t, fmt, out);
            return out;
        }
        //создает новый объект, а не модифицирует существующий
        //pos — курсор по строке, после возврата показывает, сколько разобрано
        //parse — рекурсивно восстанавливает дерево
        //if (pos != s.size()) — если после парсинга остались лишние символы — строка некорректна
        //без нее "[1:5]#garbage" превратилось бы в дерево из одного узла, а мусор остался бы незамеченным
        static BinaryTree readFromString(const std::string& s, const Traversal& t, Parser parser) {
            BinaryTree tree;
            std::size_t pos = 0;
            tree.root_ = TreeNode<T>::parse(s, pos, t, parser);
            if (pos != s.size())
                throw std::invalid_argument("extra characters after tree");
            return tree;
        }

        template <typename F>
        //принимает любую функцию/лямбду/функтор, применимую к T
        BinaryTree map(F f) const {
            BinaryTree tree;
            if (root_) tree.root_ = root_->map(f);
            return tree;
        }

        template <typename P>
        //аналогично map, но pred возвращает bool
        BinaryTree where(P pred) const {
            BinaryTree tree;
            if (root_) tree.root_ = root_->where(pred);
            return tree;
        }
        //собрать значения текущего дерева в BFS-порядке, добавить к ним значения второго, построить новое полное дерево из объединенного массива
        BinaryTree merge(const BinaryTree& other) const {
            std::vector<T> values;
            if (root_) root_->levelOrder(values);
            if (other.root_) other.root_->levelOrder(values);
            BinaryTree tree;
            tree.buildFromLevelOrder(values);
            return tree;
        }
        //найти узел со значением value через find, если не найден — исключение, клонировать поддерево с этим узлом как корнем
        BinaryTree extractSubtree(const T& value) const {
            const TreeNode<T>* node = root_ ? root_->find(value) : nullptr;
            if (!node) throw std::runtime_error("element not found");
            BinaryTree tree;
            tree.root_ = node->clone();
            return tree;
        }
        //проверить наличие элемента
        bool contains(const T& value) const {
            return root_ && root_->find(value);
        }
        //проверить наличие поддерева
        bool containsSubtree(const BinaryTree& other) const {
            if (!other.root_) return true; //пустое искомое поддерево — найдено везде
            if (!root_) return false;
            return root_->containsSubtree(other.root_);
        }
        //абсолютный путь от корня. stepByPath шагает по L/R и возвращает указатель на конечный узел
        const T& getByPath(const std::string& path) const {
            if (!root_) throw std::runtime_error("tree is empty");
            return root_->stepByPath(path)->data;
        }
        //разница с getByPath — отправная точка не корень, а произвольный узел
        const T& getByRelativePath(const T& start, const std::string& path) const {
            if (!root_) throw std::runtime_error("tree is empty");
            const TreeNode<T>* startNode = root_->find(start);
            if (!startNode) throw std::runtime_error("start element not found");
            return startNode->stepByPath(path)->data;
        }
    };

} // namespace lab1#pragma once
