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


#include <glibmm.h>
#include <vector>

#include "Font2.hpp"
#include "Buffer.hpp"
#include "NaviContext.hpp"
#include "TextContext.hpp"
#include "Text2.hpp"

namespace psc {
namespace gl {

class Diagram2
{
public:
    Diagram2(guint _size, NaviContext *naviContext, TextContext *_textCtx);
    virtual ~Diagram2();

    void setPosition(Position &pos);
    void setFont(const ptrFont2& font);
    void setName(const Glib::ustring &sname);

    void fill_data(const aptrGeom2& geom, const std::shared_ptr<Buffer<double>> &data, Gdk::RGBA *color,
	gfloat zOffs, gfloat yOffs);

    void fill_buffers(guint idx, const std::shared_ptr<Buffer<double>> &data, Gdk::RGBA *color);
    void setMaxs(const Glib::ustring &pmax, const Glib::ustring &smax);

    gfloat getDiagramHeight() const {       // y
        return m_diagramHeight;
    }
    void setDiagramHeight(gfloat _height) {
        m_diagramHeight =  _height;
    }
    gfloat getDiagramDepth() const {        // z
        return m_StripeWidth;
    }
    void setDiagramDepth(gfloat _depth) {
        m_StripeWidth = _depth;
    }
    gfloat getDiagramWidth() const {        // x
        return m_diagramWidth;
    }
    void setDiagramWidth(gfloat _width) {
        m_diagramWidth = _width;
    }
    aptrGeom2 getBase();
protected:
    gfloat m_StripeWidth;
    gfloat m_diagramHeight;
    gfloat m_diagramWidth;
    aptrGeom2 m_base;
    std::vector<psc::gl::aptrGeom2> m_values;
    ptrFont2 m_font;
    aptrText2 m_sname;
    aptrText2 m_pmax;
    aptrText2 m_smax;

    Glib::ustring m_max;
    NaviContext *naviContext;
    TextContext *m_textCtx;
    gfloat value2y(gfloat val);
    gfloat index2x(guint i);
private:
    aptrGeom2 createBase(gfloat zOffs, gfloat yOffs);
    guint m_size;       // number of data points must be shared with buffer
};


} /* namespace gl */
} /* namespace psc */