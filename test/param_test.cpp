/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2026 rpf
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
#include <optional>
#include <memory>

#include "KernelParameter.hpp"

static bool
param_test()
{
    auto params = KernelParameter::getAllParameters();
    for (auto param : params) {
        auto kernVal = param->query();
        if (kernVal.hasError()) {   // some errors are expected
            std::cout << "error " << kernVal.getError() << std::endl;
        }
        else {
            std::cout << "value  " << kernVal.getValue() << std::endl;
        }
    }
    return true;
}

int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");      // make locale aware, and make glib accept u8 const !!!
    //Gio::init();
    if (!param_test()) {
        return 1;
    }


    return 0;
}
