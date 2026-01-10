/*
 * Copyright (C) 2021 rpf
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

#include <cstdlib>
#include <stdarg.h>
#include <string>

#include "FileByLine.hpp"

FileByLine::FileByLine()
: m_file{nullptr}
, m_line{nullptr}
{

}


FileByLine::~FileByLine()
{
    if (m_file != nullptr) {
        std::fclose(m_file);
        m_file = nullptr;
    }
    if (m_line) {
        std::free(m_line);
        m_line = nullptr;
    }
}

bool
FileByLine::open(const char *_name, const char *_mode)
{
    if (m_file != nullptr) {
        std::fclose(m_file);
    }
    m_file = std::fopen(_name, _mode);
    return m_file != nullptr;
}

bool
FileByLine::open(const std::string &_name, const char *_mode)
{
    return open(_name.c_str(), _mode);
}

const char *
FileByLine::nextLine(size_t *len) {
    ssize_t read = getline(&m_line, len, m_file);
    if (read < 0) {
        return nullptr;
    }
    return m_line;
}

FILE *
FileByLine::getFile() const
{
    return m_file;
}
