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

#include <GenericGlmCompat.hpp>


#include "GpuGLquery.hpp"

//#define DEBUG 1

const std::string GpuGLqueries::prefix{"GLquery_"};
const std::string GpuGLqueries::suffixSamples{"samples"};
const std::string GpuGLqueries::suffixPrimitives{"primitives"};
std::shared_ptr<GpuGLqueries> GpuGLqueries::m_queries;

// useful type values:
//GL_SAMPLES_PASSED
//GL_PRIMITIVES_GENERATED
// more are possible but don't want to clutter display with too many options
GpuGLquery::GpuGLquery(const std::string& name, GLenum type, const std::string& unit)
: GpuCounter(name)
, m_type{type}
, m_unit{unit}
{
}

GLuint
GpuGLquery::update(Buffer<guint64>& hist, guint64 dt, int refresh_rate)
{
    GLuint value = 0;
    if (m_glQuery == 0) {
        glGenQueries(1, &m_glQuery);
    }
    else {
        glEndQuery(m_type);
        // Wait for all results to become available
        GLint available = 1, queryCount = 0;
        while (queryCount < 1000) {	// but not indefinitely
            glGetQueryObjectiv(m_glQuery, GL_QUERY_RESULT_AVAILABLE, &available);
			if (available) {
				break;
			}
            struct timespec req;// for x64 nouvea get 2 cycles
            req.tv_nsec = 1e3l; // 1e3nanos = 1usec
            req.tv_sec = 0;     //
            nanosleep(&req, nullptr);
            ++queryCount;
        }
        glGetQueryObjectuiv(m_glQuery, GL_QUERY_RESULT, &value);
	}
    glBeginQuery(m_type, m_glQuery);
    guint64 valueLPerS = (guint64)value * 1e6l / dt;    // show always per sec
    hist.set(valueLPerS);
	return value;
}

std::string
GpuGLquery::get_unit()
{
    return m_unit;
}

GpuGLqueries::GpuGLqueries()
: GpuCounters::GpuCounters()
{
}

std::list<std::string>
GpuGLqueries::list()
{
    #ifdef DEBUG
    std::cout << "GpuGLqueries::list" << std::endl;
    #endif
    std::list<std::string> names;
    names.push_back(prefix + suffixSamples);
    names.push_back(prefix + suffixPrimitives);
    return names;
}

// get a counter for given name (allow to ask for unknown names)
std::shared_ptr<GpuCounter>
GpuGLqueries::get(const std::string& name)
{
    if (name.rfind(prefix, 0) == 0) {
        if (name.rfind(suffixSamples, prefix.length()) == prefix.length()) {  // works only for ascii names
            return std::make_shared<GpuGLquery>(name, GL_SAMPLES_PASSED, "S/s");
        }
        if (name.rfind(suffixPrimitives, prefix.length()) == prefix.length()) {
            return std::make_shared<GpuGLquery>(name, GL_PRIMITIVES_GENERATED, "P/s");
        }
        std::cout << "Expected to find name " << name << "in GpuGLqueries::get" << std::endl;
    }
    return nullptr;
}

std::shared_ptr<GpuGLqueries>
GpuGLqueries::create()
{
    if (!m_queries) {
        m_queries = std::make_shared<GpuGLqueries>();
    }
    return m_queries;
}

void
GpuGLqueries::reset()
{
	m_queries.reset();
}
