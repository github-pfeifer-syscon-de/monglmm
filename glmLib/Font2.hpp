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

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_BBOX_H

#include <memory>

#include "ShaderContext.hpp"
#include "Geom2.hpp"
#include "Glyph2.hpp"
#include "active_ptr.hpp"

namespace psc {
namespace gl {

class Font2;

typedef std::shared_ptr<Font2> ptrFont2;


// unordered map will be faster on access
typedef std::unordered_map<gunichar, ptrGlyph2> Glyphs;

/*
 * Basic handling for fonts obtained from fontconfig.
 *   At the moment two faces are supported the primary is picked by name given to constructor
 *     e.g. "sans-serif" shoud be a portable choice.
 *   the second is choosen if a character is requested that is not supported by the primary face,
 *     and is selected by support for that character (there might be an option to leave this choice to freetype empty file-name?).
 */
class Font2
{
public:
    Font2(std::string name, GLfloat point_size = 24.0f); // best use with sans-serif (keeps vertex count low)
    virtual ~Font2();

    void display(std::wstring &str, ShaderContext *ctx, Matrix &perspView, Position &p, GLfloat scale, Rotational &rot);
    void displayLine(std::wstring &str, ShaderContext *ctx, Matrix &perspView, Position &pos, GLfloat scale);
    void displayTexture(std::wstring &str, ShaderContext *ctx, Matrix &perspView, Position &pos, GLfloat scale);
    // show the lower 256 glyphes
    void debug(ShaderContext *ctx, Matrix &mvp, Position &p, GLfloat scale);
    float getLineHeight();
    float getMWidth();
    GLfloat getPointSize() {
        return m_point_size;
    }
    FT_Face getDefaultFace();
    FT_Face getAlternateFace(gunichar glyph);
    // get a stored glyph or create it
    ptrGlyph2 checkGlyph(gunichar c, GeometryContext *geometryContext, GLenum textType, bool tryAlternate = true);
    void createDefault(ShaderContext *ctx, GLenum textType); // as with more glyphs we run into trouble when we create and display them right away, this function allows pre-creating all latin glyphs
private:
    FT_Face build(const std::string& path);
    std::string buildByGlyph(gunichar glyph);
    std::string buildByName(const std::string &name);
    FT_Library library;
    FT_Face m_face;
    FT_Face m_aFace;
    Glyphs glyphes;
    GLfloat m_point_size;
    mutable std::mutex m_MutexGlyph;

};





} /* namespace gl */
} /* namespace psc */