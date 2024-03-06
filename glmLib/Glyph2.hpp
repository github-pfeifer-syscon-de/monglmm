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

#include "GenericGlmCompat.hpp"

#include <glibmm.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <GL/glu.h>

#include "ShaderContext.hpp"
#include "Geometry.hpp"
#include "PositionDbl.hpp"
#include "Polygon.hpp"

namespace psc {
namespace gl {


class Glyph2
{
public:

    Glyph2(gunichar _glyph, GeometryContext *geometryContext);
    ~Glyph2();

    bool operator==(gunichar other) {
        return glyph == other;
    }
    GLfloat getAdvance() {
        return m_advance;
    }
    void displayLine(Matrix &mv);
    void display(Matrix &mv);
    GLint bindTexture();

    bool extractGlyph(FT_Library library, FT_Face face, FT_UInt glyph_index, GLenum textType);
    void addFillPoint(const Position& pos, const Color &c, const Vector &n);
    GLuint getNumVertex();
    gunichar getChar() {
        return glyph;
    }
protected:
    gunichar glyph;
    GLfloat m_advance;
    GLfloat m_height;
    Geometry m_lineGeom;
    Geometry m_fillGeom;
    GLuint   m_tex;
    GeometryContext *m_ctx;
private:
    void render2tex(FT_Library library, FT_Face face, FT_UInt glyph_index);
    void buildLineTriangels(FT_Library library, FT_Face face, FT_UInt glyph_index);

    bool tesselate(p2t::Polygons lines);
    void setAdvance(GLfloat advance);
    void setHeight(GLfloat height);
    void addLine(std::shared_ptr<PositionDbl> pos, std::shared_ptr<PositionDbl> end, Color &c, Vector *n);
    void create_vao();
};


typedef std::shared_ptr<Glyph2> ptrGlyph2;

} /* namespace gl */
} /* namespace psc */