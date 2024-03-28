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


#include "Diagram2.hpp"

namespace psc {
namespace gl {


#include <stdlib.h>
#include <cmath>
#include <iostream>

#include "GenericGlmCompat.hpp"
#include "Diagram2.hpp"

Diagram2::Diagram2(guint _size, NaviContext *_naviContext, TextContext *_textCtx)
: m_StripeWidth{0.3f}
, m_diagramHeight{0.7f}
, m_diagramWidth{2.0f}
, m_values()
, naviContext{_naviContext}
, m_textCtx{_textCtx}
, m_size{_size}
{
}


Diagram2::~Diagram2()
{
    m_values.clear();
}

gfloat
Diagram2::index2x(guint i)
{
    return ((gfloat) i / (gfloat)(m_size-1)) * m_diagramWidth;
}

gfloat
Diagram2::value2y(gfloat val)
{
    return val * m_diagramHeight;
}

void
Diagram2::setPosition(Position &pos)
{
    auto base = getBase();
    auto lbase = base.lease();
    if (lbase) {
        lbase->setPosition(pos);
    }
}

void
Diagram2::setFont(const ptrFont2& font)
{
    m_font = font;
}

void
Diagram2::setName(const Glib::ustring &sname)
{
    if (!m_sname) {
        m_sname = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, naviContext, m_font);
        auto lsname = m_sname.lease();
        if (lsname) {
            lsname->setTextContext(m_textCtx);
            Position ptn(0.0f, m_diagramHeight - m_font->getLineHeight() * 0.007f, 0.0f);
            lsname->setPosition(ptn);
            lsname->setScale(0.010f);
            lsname->setName(Glib::ustring::sprintf("m_sname %s", sname));
        }
        auto base = getBase();
        auto lbase = base.lease();
        if (lbase) {
            lbase->addGeometry(m_sname);
        }
    }
    auto lsname = m_sname.lease();
    if (lsname) {
        lsname->setText(sname);
    }
}

void
Diagram2::fill_data(
        const aptrGeom2& geom
      , const std::shared_ptr<Buffer<double>> &data
      , Gdk::RGBA *color
      , gfloat zOffs, gfloat yOffs)
{
    auto lgeom = geom.lease();
    if (lgeom) {
        Color c((gfloat)color->get_red(), (gfloat)color->get_green(), (gfloat)color->get_blue());
        lgeom->deleteVertexArray();
        for (guint i = 0; i < m_size - 1; ++i) {
            Position p1(index2x(i    ), value2y(data->get(i    )) + yOffs, zOffs);
            Position p2(index2x(i + 1), value2y(data->get(i + 1)) + yOffs, zOffs + m_StripeWidth);
            lgeom->addRect(p1, p2, c);
        }
        lgeom->create_vao();
        checkError("Error create vao data");
    }
}

aptrGeom2
Diagram2::createBase(gfloat zOffs, gfloat yOffs)
{
    auto geom = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, naviContext);
    auto lgeom = geom.lease();
    if (lgeom) {
        // Base
        Color c(0.2f, 0.2f, 0.2f);
        Position p1(index2x(0       ), value2y(0.f)-0.002f, zOffs);
        Position p2(index2x(m_size-1), value2y(0.f)-0.002f, zOffs + m_StripeWidth);
        lgeom->addRect(p1, p2, c);

        Position p3(index2x(0), value2y(0.f) - 0.002f, zOffs);
        Position p4(index2x(0), value2y(1.0f) - 0.002f, zOffs + m_StripeWidth);
        lgeom->addRect(p3, p4, c);
        lgeom->create_vao();
        Glib::ustring name{"base"};
        if (m_sname) {
            auto lname = m_sname.lease();
            if (lname) {
                name += lname->getText();
            }
        }
        lgeom->setName(name);
        checkError("Error create vao base");
    }
    return geom;
}

void
Diagram2::fill_buffers(guint idx, const std::shared_ptr<Buffer<double>> &data, Gdk::RGBA *color)
{
    auto base = getBase();
    auto lbase = base.lease();
    if (lbase) {
        while (m_values.size() <= idx) {
            auto geom = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, naviContext);
            lbase->addGeometry(geom);
            m_values.push_back(geom);
        }
        auto geom = m_values[idx];
        fill_data(geom, data, color, 0.0f, -0.001f*idx);
        auto lgeom = geom.lease();
        if (lgeom) {
            lgeom->setName(Glib::ustring::sprintf("data %d %s", idx, m_sname.get()->getText()));
        }
    }
}

aptrGeom2
Diagram2::getBase()
{
    if (!m_base) {
        m_base = createBase(0.0f, 0.0f);
    }
    return m_base;
}

void
Diagram2::setMaxs(const Glib::ustring &pmax, const Glib::ustring &smax)
{
    if (!m_pmax) {
        m_pmax = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, naviContext, m_font);
        auto lpmax = m_pmax.lease();
        if (lpmax) {
            lpmax->setTextContext(m_textCtx);
            Position ptm(-1.0f, m_diagramHeight - m_font->getLineHeight() * 0.004f, 0.0f);
            lpmax->setPosition(ptm);
            lpmax->setScale(0.006f);
            lpmax->setName("m_pmax");
            auto base = getBase();
            auto lbase = base.lease();
            if (lbase) {
                lbase->addGeometry(m_pmax);
            }
        }
    }
    if (!m_smax) {
        m_smax = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, naviContext, m_font);
        auto lsmax = m_smax.lease();
        if (lsmax) {
            lsmax->setTextContext(m_textCtx);
            Position pti(-1.0f, m_diagramHeight - 2.0f*m_font->getLineHeight() * 0.004f, 0.0f);
            lsmax->setPosition(pti);
            lsmax->setScale(0.006f);
            lsmax->setName("m_smax");
            auto base = getBase();
            auto lbase = base.lease();
            if (lbase) {
                lbase->addGeometry(m_smax);
            }
        }
    }
    auto lpmax = m_pmax.lease();
    if (lpmax) {
        lpmax->setText(pmax);
    }
    auto lsmax = m_smax.lease();
    if (lsmax) {
        lsmax->setText(smax);
    }
    //Position &pos = getBase()->getPos();
    //Position ptn(pos.x-1.0f, pos.y + m_diagramHeight - m_font->getLineHeight() * 0.007f, pos.z);
    //m_font->display(m_sname, naviContext, persView, ptn, 0.010f);
    //checkError("display name");
//    Position ptm(pos.x-2.0f, pos.y + m_diagramHeight - m_font->getLineHeight() * 0.004f, pos.z);
//    m_font->display(pmax, naviContext, persView, ptm, 0.006f);
//    checkError("display pmax");
//    Position pti(pos.x-2.0f, pos.y + m_diagramHeight - 2.0f*m_font->getLineHeight() * 0.004f, pos.z);
//    m_font->display(smax, naviContext, persView, pti, 0.006f);
//    checkError("display smax");
}



} /* namespace gl */
} /* namespace gl */