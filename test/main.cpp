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
#include <Prim.hpp>
#include <Simple.hpp>

#include "main.hpp"
#include "active_ptr.hpp"

std::ostream&
operator<< (std::ostream& os, const std::source_location& location)
{
    os << location.file_name() << "("
       << location.line() << ":"
       << location.column() << ") "
       << location.function_name() << ";";
    return os;
}

Base::Base()
: m_val{0}
{
}

Base::~Base()
{
}

void
Base::dummy()
{
    std::cout << std::source_location::current() << std::endl;
}

void
Base::base()
{
    std::cout << std::source_location::current() << std::endl;
}

Test::Test(int n)
: Base()
, m_n{n}
{
    std::cout << std::source_location::current() << " n " << m_n << std::endl;
}

Test::~Test()
{
    std::cout << std::source_location::current() << " n " << m_n << std::endl;
}

void
Test::dummy()
{
    std::cout << std::source_location::current() << std::endl;
}

void
Test::test()
{
    std::cout << std::source_location::current() << " n " << m_n << std::endl;
    ++m_n;
}

int Test::get()
{
    return m_n;
}

static void
funct(const psc::mem::active_ptr<Base>& base2) noexcept
{
    std::cout << "funct base " << base2 << std::endl;
    auto baseLease = base2.lease();
    if (baseLease) {
        baseLease->dummy();
        //std::cout << baseLease->get() << std::endl;
    }
    else {
        std::cout << "funct base no access"  << std::endl;
    }
}

static void
ptrTest()
{
    std::cout << "Start" << std::endl;
    //Test* ttest = new Test(8);
    // this will be 4 bytes higher than m_val
    //std::cout << "ttest " << std::hex << &ttest->m_n << std::dec << std::endl;
    //Base* tbase = dynamic_cast<Base*>(ttest);
    //std::cout << "tbase " << std::hex << &tbase->m_val << std::dec << std::endl;
    //std::cout << "----------------" << std::endl;
    {
        psc::mem::active_ptr<Test> test;
        {
            std::cout << "---- before make_active " << (test ? "valid" : "invalid") << std::endl;
            test = psc::mem::make_active<Test>(7);
            std::cout << "---- after make_active " << test << std::endl;
            {
                auto testLease = test.lease();
                if (testLease) {
                    testLease->test();
                    std::cout << "test get " << testLease->get() << std::endl;
                }
            }
            {
                funct(test);
                {
                    std::cout << "---- copy cc " << std::endl;
                    auto test2{test};  // will use implicite copy
                    std::cout << "---- after 2inst " << test << std::endl;
                    //test2 = test; uses assignment
                    {
                        auto lease1 = test2.lease();
                        if (lease1) {
                            std::cout << "test2 get " << lease1 << std::endl;
                            lease1->test();
                            test2.reset();
                            lease1->test();
                        }
                        else {
                            std::cout << "No access lease1 " << std::endl;
                        }
                    }
                }
                std::cout << "---- after test2 decayed  " << test << std::endl;
                {
                    auto lease2 = test.lease();
                    if (lease2) {
                        lease2->test();
                    }
                    else {
                        std::cout << "No access lease2" << std::endl;
                    }
                }
                std::cout << "---- closing test2" << test << std::endl;
            }
        }
        std::cout << "--- testing valid " << (test ? "valid" : "invalid") << std::endl;
        std::cout << "End" << std::endl;
    }
}


static void
primTest()
{
    //Prim prim{1000000l};
    //prim.eratosthenes();
    //prim.dijkstra();
    //dijkstraSimple(1000000);
}

static void
simpleTest()
{
    std::cout << "--- make_active 2" << std::endl;
    auto simple{make_active<Test>(2)};
    std::cout << "--- make_active 3" << std::endl;
    auto simpler{make_active<Test>(3)};
    std::cout << "--- use lval 3" << std::endl;
    Simple<Test> simple4{simpler};
    std::cout << "--- use rval 3" << std::endl;
    Simple<Test> simple5{std::move(simpler)};
    std::cout << "--- use operator= 2" << std::endl;
    simpler = simple;
    simpler->test();
    Simple<Base> simpelb(simple);
    simpelb->dummy();
}

int
main(int argc, char** argv) {
    ptrTest();
    //primTest();
    //simpleTest();

    return 0;
}
