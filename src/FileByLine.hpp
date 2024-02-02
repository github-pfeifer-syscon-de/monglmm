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

#pragma once

#include <string>
#include <stdio.h>

// encapulate FILE to ensure close & free

class FileByLine {
public:
    FileByLine();
    virtual ~FileByLine();

    bool open(const char *_name, const char *_mode);
    bool open(const std::string &_name, const char *_mode);
    const char * nextLine(size_t *len);
    FILE *getFile() const;

private:
    FILE *m_file;
    char *m_line;
};

