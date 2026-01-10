/*
 * Copyright (C) 2022 PPf <gnu3@pfeifer-syscon.de>
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

#include <list>
#include <memory>
#include <string>

#include "HistMonitor.hpp"


// represents a single value
class GpuCounter {
public:
    GpuCounter(const std::string& name);

    virtual GLuint update(Buffer<guint64>& hist, guint64 dt, int refresh_rate) = 0;
    virtual std::string get_unit() = 0;
    std::string get_name();
protected:
    const std::string m_name;
private:

};

// represent all possible values for a source
class GpuCounters {
public:
    GpuCounters();

    // list available counters (names should be unique, so use a prefix)
    virtual std::list<std::string> list() = 0;
    // get a counter for given name (allow to ask for unknown names)
    virtual std::shared_ptr<GpuCounter> get(const std::string& name) = 0;
private:

};
