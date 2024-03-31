/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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
#include <atomic>
#include <mutex>
#include <iostream>
#include <type_traits>
#include <tuple>        // boolalpha

// Nothing secfic just some playground, to check some stuff

class Base
{
public:
    Base()
    {
        std::cout << "Base::Base" << std::endl;
    }
    virtual ~Base()
    {
        std::cout << "Base::~Base" << std::endl;
    }

};

class Test
: public Base
{
public:
    Test()
    //: m_usecount{}
    //, m_changeMutex{}
    {
        std::cout << "Test::Test" << std::endl;
    }
    virtual ~Test() {
        std::cout << "Test::~Test" << std::endl;
    }
    void fun() {
        std::lock_guard<std::mutex> lock(m_changeMutex);
        ++m_usecount;
        std::cout << "Test::fun use " << m_usecount << std::endl;
    }
private:
    std::atomic_uint32_t m_usecount;
    std::mutex m_changeMutex;
};



struct A {
public:
    A() = default;
    ~A() = default;
    explicit A(const A& a) = delete;
};

struct B {
public:
    B() = default;
    ~B() = default;
    B(const B& b) = default;
private:
    B& operator=( B const& ) = default;
};

struct C
: public A {
public:
    C() = default;
    ~C() = default;
    C(const C& b) = default;
private:
    C& operator=( C const& ) = default;
};

template< class Type >
bool isAssignable() {
    return std::is_assignable< Type,Type >::value;
}

template< class Type >
bool isCopyAssignable() {
    return std::is_copy_assignable< Type >::value;
}

template< class Type >
bool isCopyConstructible() {
    return std::is_copy_constructible< Type >::value;
}


template< class TypeOf, class Type >
bool isBaseOf() {
    return std::is_base_of< TypeOf, Type >::value;
}

int
main(int argc, char** argv)
{
    std::cout << "simple" << std::endl;
    std::allocator<Test> alloc;
    using traits_t = std::allocator_traits<decltype(alloc)>; // The matching trait
    Test* test = traits_t::allocate(alloc, 1);         // with count
    traits_t::construct(alloc, test);     // construct the element
    test->fun();
    // even if the allocator is wrong, destroy will include test
    std::allocator<Base> baseAlloc;
    using baseTraits_t = std::allocator_traits<decltype(baseAlloc)>; // The matching trait
    baseTraits_t::destroy(baseAlloc, test);
    baseTraits_t::deallocate(baseAlloc, test, 1);
    //int* p  = new int[10];
    //for (int i = 0; i < 10; ++i) {
    //    p[i] = 0;
    //}
    //char* c = reinterpret_cast<char*>(p);
    //delete[] p;

    std::cout << "template -------------" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "assign A " << isAssignable< A >() << std::endl;              // OK.
    std::cout << "assign B " << isAssignable< B >() << std::endl;              // Uh oh.
    std::cout << "copy assign A " << isCopyAssignable< A >() << std::endl;              // OK.
    std::cout << "copy assign B " << isCopyAssignable< B >() << std::endl;              // Uh oh.
    std::cout << "copy constr A " << isCopyConstructible< A >() << std::endl;              // OK.
    std::cout << "copy constr B " << isCopyConstructible< B >() << std::endl;              // Uh oh.
    std::cout << "base of A<-B " << isBaseOf< A,B >() << std::endl;              // OK.
    std::cout << "base of A<-C " << isBaseOf< A,C >() << std::endl;              // Uh oh.

    std::cout << "end" << std::endl;
    return 0;
}