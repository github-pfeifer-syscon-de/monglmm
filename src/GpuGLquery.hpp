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

#include <memory>

#include "GpuCounter.hpp"

class GpuGLquery : public GpuCounter {
public:
    GpuGLquery(const std::string& name, GLenum type, const std::string& unit);

    GLuint update(Buffer<guint64>& hist, guint64 dt, int refresh_rate) override;
    std::string get_unit() override;
protected:
    GLenum m_type;
    GLuint m_glQuery{0};
    std::string m_unit;
private:
};

class GpuGLqueries : public GpuCounters {
public:
    GpuGLqueries();
    std::list<std::string> list() override;
    // get a counter for given name (allow to ask for unknown names)
    std::shared_ptr<GpuCounter> get(const std::string& name) override;
    static std::shared_ptr<GpuGLqueries> create();
    static void reset();
private:
    static std::shared_ptr<GpuGLqueries> m_queries;
    static const std::string prefix;
    static const std::string suffixSamples;
    static const std::string suffixPrimitives;
};
