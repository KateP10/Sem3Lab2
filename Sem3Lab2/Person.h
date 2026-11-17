#pragma once

#include <ostream>   //std::ostream для operator<<
#include <string>    //std::string
#include <utility>   //std::move

namespace lab2 {

    class Person {
    public:
        Person() = default; //конструктор по умолчанию
        Person(int id,
            std::string firstName,
            std::string middleName,
            std::string lastName,
            int birthYear)
            : id_(id)
            , firstName_(std::move(firstName))
            , middleName_(std::move(middleName))
            , lastName_(std::move(lastName))
            , birthYear_(birthYear) {
        }

        int GetId() const { return id_; } //возврат по значению
        const std::string& GetFirstName()  const { return firstName_; }
        const std::string& GetMiddleName() const { return middleName_; }
        const std::string& GetLastName()   const { return lastName_; }
        int GetBirthYear() const { return birthYear_; }
        //ФИО полностью, через пробелы
        std::string GetFullName() const {
            return firstName_ + " " + middleName_ + " " + lastName_;
        }
        //Фамилия И.О.
        std::string GetFIO() const {
            //std::string(1, middleName_[0]) — строка из одного символа
            std::string m = middleName_.empty() ? std::string() : std::string(1, middleName_[0]) + "."; 
            std::string l = lastName_.empty() ? std::string() : std::string(1, lastName_[0]) + ".";
            return lastName_ + " " + m + l;
        }
        int GetAge(int year) const { return year - birthYear_; } //возраст на заданный год

    private:
        int id_ = 0; //уникальный идентификатор
        std::string firstName_, middleName_, lastName_; //ФИО по частям
        int birthYear_ = 0;
    };
    //operator<<
    //inline - защита от множественного включения, потому что определение в заголовке
    inline std::ostream& operator<<(std::ostream& os, const Person& p) {
        os << p.GetId() << ":" << p.GetFullName() << ":" << p.GetBirthYear();
        return os;
    }

} // namespace lab2