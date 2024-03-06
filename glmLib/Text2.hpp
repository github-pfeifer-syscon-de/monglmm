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

class TextContext;

#include "Geom2.hpp"
#include "Font2.hpp"


namespace psc {
namespace gl {

class Text2;

typedef psc::mem::active_ptr<Text2> aptrText2;

class Text2
: public psc::gl::Geom2
{
public:
    // this may get used with type
    //   TRINAGLES the glyphs are build from triangles
    //   LINES for debugging
    //   QUADS the glyphs are build from textures
    //      (and drawn without writing the z-buffer (need to be last while drawing)
    //         and may get artefacts from that)
    Text2(GLenum type, NaviContext *ctx, const psc::gl::ptrFont2& font);
    virtual ~Text2() = default;

    void setText(const Glib::ustring &text);
    const Glib::ustring &getText();
    void setScalePos(Position &pos, float _scale);
    void display(const Matrix &mvp) override;
    void display(NaviContext* context, const Matrix &projView) override;

    // allow assigning a context for rendering text
    //    -> support color, simpler shader, not influenced by light
    void setTextContext(TextContext *textCtx);
private:
    void prepareTexture();
    void finishTexture();

    Glib::ustring m_text;
    psc::gl::ptrFont2 m_font;
    NaviContext *m_shaderCtx;
    TextContext *m_textCtx;
    // Used to display texture
    GLint m_texID;
    GLenum m_textType;
};


} /* namespace gl */
} /* namespace psc */