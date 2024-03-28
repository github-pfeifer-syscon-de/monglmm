/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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


#include "TreeRenderer2.hpp"

namespace psc {
namespace gl {



void
TreeRenderer2::setFullWidth(double _fullWidth)
{
    m_fullWidth = _fullWidth;
}

double
TreeRenderer2::getFullWidth()
{
    return m_fullWidth;
}

Color
TreeRenderer2::pos2Color(double pos, TreeNodeState stage, float load)
{
    float coffs = 0.2f + (float)pos / (m_fullWidth * 1.2);   // make adjacent piece distinguishable by color (mid is 0 .. 2pi)
    Color c;
    switch (stage) {
        case TreeNodeState::Running:
            c.r = 0.2f + load;        // if running show load as color, green ... blue
            c.g = 1.0f - coffs;
            c.b = coffs;
            break;
        case TreeNodeState::Finished:
        case TreeNodeState::Close:
            c.r = 1.0f;
            c.g = 0.0f;
            c.b = 0.0f;
            break;
        default:
            c.r = coffs;    // started is greenish
            c.g = 1.f;
            c.b = 1.0f - coffs;
            break;
    }

    return c;
}





} /* namespace gl */
} /* namespace psc */