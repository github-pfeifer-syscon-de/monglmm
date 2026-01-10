/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2024 RPf 
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

#pragma once

#include <map>
#include <iostream>
#include <memory>

#include "Process.hpp"

static const long ROOT_PID = 1l;

class ProcessesBase
{
public:
    ProcessesBase();
    virtual ~ProcessesBase();

    void update();
    void buildTree();
    pProcess findPid(long pid);
protected:
    std::map<long, pProcess> mProcesses;
    constexpr static auto sdir = "/proc";
    virtual pProcess createProcess(std::string path, long pid);
    pProcess m_procRoot;

private:

};

