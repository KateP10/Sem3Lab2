#pragma once

#include "AvlTree.h"
#include "Person.h"

#include <string>      //std::string в ключах
#include <tuple>       //std::tuple, std::make_tuple
#include <utility>     //std::pair (дл€ range), std::move
#include <vector>      //std::vector<Person>, std::vector<PersonPtr>

//индексирование данных
//составной индекс строит ключ сразу из нескольких атрибутов Ч std::tuple<FirstName, LastName, BirthYear> Ч и дает поиск за один find

namespace lab2 {

    using PersonPtr = const Person*; //индекс хранит указатели на неизмен€емые объекты

    class Index {
    public:
        void build(const std::vector<Person>& persons) {
            data_ = persons;
            byFirstName_ = {};
            byBirthYear_ = {};
            byFirstLastYear_ = {};
            byFullNameAge_ = {};
            //GetFirstName() возвращает const std::string& Ч ссылку на внутреннее поле
            //insertOrAppend принимает const K&, копирует строку в ключ узла (если узла еще нет) или дописывает &p в вектор существующего узла
            //&p Ч адрес элемента в data_, стабильный до тех пор, пока data_ не перестроитс€
            //используем insertOrAppend, а не insert: insert перезаписал бы значение, оставив только последнее
            for (const auto& p : data_) {
                byFirstName_.insertOrAppend(p.GetFirstName(), &p);
                byBirthYear_.insertOrAppend(p.GetBirthYear(), &p);
                byFirstLastYear_.insertOrAppend(std::make_tuple(p.GetFirstName(), p.GetLastName(), p.GetBirthYear()), &p); //составной ключ
                byFullNameAge_.insertOrAppend(std::make_tuple(p.GetFullName(), p.GetAge(2019)), &p);
            }
        }
        //поиск по одному атрибуту
        std::vector<PersonPtr> findByFirstName(const std::string& s) const {
            //byFirstName_.find(s) возвращает const std::vector<PersonPtr>* Ч указатель на значение узла или nullptr
            const auto* v = byFirstName_.find(s); 
            return v ? *v : std::vector<PersonPtr>{}; //возвращаем копию вектора (*v)
        }
        std::vector<PersonPtr> findByBirthYear(int y) const {
            const auto* v = byBirthYear_.find(y);
            return v ? *v : std::vector<PersonPtr>{};
        }

        //диапазон
        std::vector<PersonPtr> findByBirthYearRange(int lo, int hi) const {
            //range(lo, hi) возвращает std::vector<std::pair<int, std::vector<PersonPtr>>> Ч все пары с ключом в [lo, hi], отсортированные по ключу
            auto pairs = byBirthYear_.range(lo, hi);
            std::vector<PersonPtr> out;
            //расплющиваем вектор пар в один вектор указателей
            for (const auto& pr : pairs) out.insert(out.end(), pr.second.begin(), pr.second.end());
            return out;
        }

        //составные атрибуты
        std::vector<PersonPtr> findByFirstLastYear(const std::string& first, const std::string& last, int year) const {
            const auto* v = byFirstLastYear_.find(std::make_tuple(first, last, year));
            return v ? *v : std::vector<PersonPtr>{};
        }
        std::vector<PersonPtr> findByFullNameAge(const std::string& fullName, int age2019) const {
            const auto* v = byFullNameAge_.find(std::make_tuple(fullName, age2019));
            return v ? *v : std::vector<PersonPtr>{};
        }

        std::size_t totalRecords() const { return data_.size(); } //количество записей в хранилище
        //возвращает const& на исходную коллекцию Ч чтобы вызывающий мог посмотреть данные в меню и в тестах, не копиру€
        const std::vector<Person>& data() const { return data_; }

    private:
        std::vector<Person> data_; //основное хранилище. ¬ладеет объектами; индексы держат const Person* на его элементы
        //индекс по FirstName.  люч Ч строка, значение Ч вектор указателей на всех людей с этим именем
        AvlTree<std::string, std::vector<PersonPtr>> byFirstName_; 
        AvlTree<int, std::vector<PersonPtr>> byBirthYear_; //по BirthYear
        AvlTree<std::tuple<std::string, std::string, int>, std::vector<PersonPtr>> byFirstLastYear_; //FirstName + LastName + BirthYear
        AvlTree<std::tuple<std::string, int>, std::vector<PersonPtr>> byFullNameAge_; //FullName + Age(2019)
    };

} // namespace lab2