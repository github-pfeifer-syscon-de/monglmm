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
    int* p  = new int[10];
    for (int i = 0; i < 10; ++i) {
        p[i] = 0;
    }
    //char* c = reinterpret_cast<char*>(p);
    delete[] p;

    std::cout << "end" << std::endl;
    return 0;
}