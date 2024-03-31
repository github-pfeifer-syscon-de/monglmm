/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include "Font2.hpp"
#include "NaviContext.hpp"
#include "TextContext.hpp"
#include "TreeNode2.hpp"
#include "TreeGeometry2.hpp"


namespace psc {
namespace gl {

class ColorScheme
{
public:
    ColorScheme() = default;
    virtual ~ColorScheme() = default;
    Color pos2Color(float pos, float fullWidth, TreeNodeState stage, float load, bool isPrim)
    {
        Color c;
        switch (stage) {
        case TreeNodeState::Running:
            c = runningColor(pos, fullWidth, load, isPrim);
            break;
        case TreeNodeState::Finished:
        case TreeNodeState::Close:
            c.r = 1.0f;
            c.g = 0.0f;
            c.b = 0.0f;
            break;
        default:
            c.r = 0.0;    // started is greenish
            c.g = 1.0f;
            c.b = 0.0f;
            break;
        }
        return c;
    }
protected:
    virtual Color runningColor(float pos, float fullWidth, float load, bool isPrim) = 0;
};

class ColorSchemeGradient
: public ColorScheme
{
public:
    ColorSchemeGradient() = default;
    virtual ~ColorSchemeGradient() = default;
protected:
    Color runningColor(float pos, float fullWidth, float load, bool isPrim)
    {
        float coffs = 0.2f + (float)pos / (fullWidth * 1.2);   // make adjacent piece distinguishable by color (mid is 0 .. 2pi)
        Color c;
        c.r = 0.2f + load;        // if running show load as color, green ... blue
        c.g = 1.0f - coffs;
        c.b = coffs;
        return c;
    }

};

class ColorSchemePrimary
: public ColorScheme
{
public:
    ColorSchemePrimary() = default;
    virtual ~ColorSchemePrimary() = default;
protected:
    Color runningColor(float pos, float fullWidth, float load, bool isPrim)
    {
        Color c;
        c.r = 0.2f + load;        // if running show load as color, green ... blue
        if (isPrim) {
            c.g = 0.8f;
            c.b = 0.2f;
        }
        else {
            c.g = 0.2f;
            c.b = 0.8f;
        }
        return c;
    }

};

class TreeRenderer2
{
public:
    TreeRenderer2() = default;
    virtual ~TreeRenderer2() = default;

    virtual psc::mem::active_ptr<TreeGeometry2> create(
          const std::shared_ptr<TreeNode2>& _root
        , NaviContext* _ctx
        , TextContext* textCtx
        , const psc::gl::ptrFont2& font) = 0;
    virtual double getDistIncrement() = 0;

    void setFullWidth(float _fullWidth);
    double getFullWidth();

    Color pos2Color(double pos, TreeNodeState stage, float load, bool isPrim);

private:
    float m_fullWidth;
    ColorSchemeGradient colorScheme;
};



} /* namespace gl */
} /* namespace psc */