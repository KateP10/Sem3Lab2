#pragma once

#include <array>
#include <string>

namespace lab1 {
    //Symbol - три возможных действи€ при обходе, Traversal	- пор€док трех действий 
    //enum class Ч scoped enumeration. “ри значени€: Symbol::Root, Symbol::Left, Symbol::Right. ¬нутри компил€тора Ч числа 0, 1, 2
    enum class Symbol { Root, Left, Right };

    struct Traversal {
        std::array<Symbol, 3> order; //упор€доченна€ тройка
    };
    //constexpr Ч вычисление на этапе компил€ции
    //константа попадает пр€мо в бинарник, без инициализации при старте программы; 
    //ее можно использовать там, где нужно compile - time значение(например, в constexpr функци€х, в размерах массивов Ч если бы понадобилось)
    //Traversal Ч это struct с одним полем order, инициализируемым через brace-init
    //то есть внешние {} инициализируют Traversal, внутренние Ч его поле order типа std::array
    constexpr Traversal KLP{ {Symbol::Root,  Symbol::Left,  Symbol::Right} };
    constexpr Traversal KPL{ {Symbol::Root,  Symbol::Right, Symbol::Left } };
    constexpr Traversal LPK{ {Symbol::Left,  Symbol::Right, Symbol::Root } };
    constexpr Traversal LKP{ {Symbol::Left,  Symbol::Root,  Symbol::Right} };
    constexpr Traversal PLK{ {Symbol::Right, Symbol::Left,  Symbol::Root } };
    constexpr Traversal PKL{ {Symbol::Right, Symbol::Root,  Symbol::Left } };
    //inline здесь об€зателен: функци€ определена в заголовке, а заголовок включаетс€ в несколько .cpp
    //без inline линкер увидит несколько определений и упадЄт с multiple definition
    inline std::string traversalName(const Traversal& t) {
        if (t.order == KLP.order) return "KLP";
        if (t.order == KPL.order) return "KPL";
        if (t.order == LPK.order) return "LPK";
        if (t.order == LKP.order) return "LKP";
        if (t.order == PLK.order) return "PLK";
        if (t.order == PKL.order) return "PKL";
        return "?";
    }

} // namespace lab1