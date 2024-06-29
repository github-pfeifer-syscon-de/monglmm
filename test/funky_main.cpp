/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2023 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <source_location>
#include <active_ptr.hpp>
#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif

#include "funky_main.hpp"




struct A {

    virtual void name()
    {
        std::cout << "A" << std::endl;
    }
};

struct B
: public A {
    virtual void name() override
    {
        std::cout << "B" << std::endl;
    }

};

template<typename T>
struct TypeName {
    constexpr static std::string_view fullname_intern() {
        #if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
        #elif defined(_MSC_VER)
            return __FUNCSIG__;
        #else
            #error "Unsupported compiler"
        #endif
    }
    constexpr static std::string_view name() {
        size_t prefix_len = TypeName<void>::fullname_intern().find("void");
        size_t multiple   = TypeName<void>::fullname_intern().size() - TypeName<int>::fullname_intern().size();
        size_t dummy_len  = TypeName<void>::fullname_intern().size() - 4*multiple;
        size_t target_len = (fullname_intern().size() - dummy_len)/multiple;
        std::string_view rv = fullname_intern().substr(prefix_len, target_len);
        if (rv.rfind(' ') == rv.npos)
            return rv;
        return rv; //.substr(rv.rfind(' ')+1);
    }

    using type = T;
    constexpr static std::string_view value = name();
};

static void
simpleTest()
{
    std::cout << "simple" << std::endl;
    auto b = psc::mem::make_active<B>();
    if (auto lb = b.lease()) {
        lb->name();
    }
    std::cout << typeid(b).name() << std::endl;
    auto p_a = psc::mem::dynamic_pointer_cast<A>(b);
    if (auto lp_a = p_a.lease()) {
        lp_a->name();
        std::cout << typeid(lp_a).name() << std::endl;
        //auto demang = abi::__cxa_demangle(typeid(lp_a.get()).name(), nullptr,
        //                                   nullptr, nullptr);
        //std::cout << demang << std::endl;
        std::cout << TypeName<decltype(p_a)>::value << std::endl;
        auto b = dynamic_cast<B*>(p_a.get());
        std::cout << TypeName<decltype(b)>::value << std::endl;

        //free(demang);
    }
    std::cout << typeid(p_a).name() << std::endl;
    std::cout << "~simple" << std::endl;
}

int
main(int argc, char** argv) {
    simpleTest();

    return 0;
}
