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


#pragma once

#include "NaviContext.hpp"


class GraphShaderContext : public NaviContext
{
public:
    GraphShaderContext();
    virtual ~GraphShaderContext();

    void setLight();
    virtual void updateLocation() override;

    bool useNormal() override;
    bool useColor() override;
    bool useUV() override;

protected:
private:
    GLint m_light_location;
    //GLint m_pos_location;

    Vector light;	/* light */
    //Vector pos;		/* element position */
};

