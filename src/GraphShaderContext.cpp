/*
 * Copyright (C) 2018 rpf
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
#include <iostream>
#include <glibmm.h>
#include <math.h>

#include "glarea-error.h"
#include "ShaderContext.hpp"
#include "GraphShaderContext.hpp"

GraphShaderContext::GraphShaderContext()
: NaviContext::NaviContext{}
, m_light_location{0}
, light{0.f, -1.f, 0.f}
//, m_pos_location(0)
{
}



GraphShaderContext::~GraphShaderContext()
{
}

void
GraphShaderContext::updateLocation()
{
    NaviContext::updateLocation();

    m_light_location = glGetUniformLocation(m_program, "light");
}



void
GraphShaderContext::setLight()
{
    glUniform3fv(m_light_location, 1, &light[0]);
    checkError("glUniform3fv (light)");
}

bool GraphShaderContext::useNormal() {
    return true;
}

bool GraphShaderContext::useColor() {
    return true;
}

bool GraphShaderContext::useUV() {
    return false;
}