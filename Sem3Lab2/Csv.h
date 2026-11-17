#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace lab2 {
    //сериализатор таблицы vector<vector<string>> в CSV-файл, разделитель ;
    //возвращает bool: false Ч если файл не открылс€; true Ч если все записалось
    inline bool saveCsv(const std::string& path, const std::vector<std::vector<std::string>>& rows) {
        std::ofstream out(path);
        if (!out) return false;
        for (const auto& row : rows) {
            for (std::size_t i = 0; i < row.size(); ++i) {
                if (i) out << ';';
                out << row[i];
            }
            out << '\n';
        }
        return true;
    }

} // namespace lab2