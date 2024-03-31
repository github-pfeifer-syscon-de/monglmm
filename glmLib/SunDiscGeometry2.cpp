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

#include <cmath>

#include "SunDiscGeometry2.hpp"

namespace psc {
namespace gl {



SunDiscGeometry2::SunDiscGeometry2(const psc::gl::ptrFont2& font, GeometryContext *_ctx)
: TextTreeGeometry2::TextTreeGeometry2(font, _ctx)
{
    setRemoveChildren(false);   // as we keep track of children in our structure (and need them to keep transform) do not delete them
}

void
SunDiscGeometry2::update(double inner, double start, double size, Position &pos, const std::shared_ptr<TreeNode2>& treeNode)
{
    m_start = start;
    m_size = size;
    m_inner = inner;
    m_stage = treeNode->getStage();
    m_load = treeNode->getLoad();
    m_pos = pos;

}

void
SunDiscGeometry2::create(
          const std::shared_ptr<TreeNode2>& node
        , SunDiscRenderer2 &_renderer)
{
    short idx = 0;
    double outer = m_inner + _renderer.getDistIncrement() - _renderer.getRadiusGap();
    double end = m_start + m_size;
    double step = M_PI * 2.0 / 60.0;   // approximate circle by 60 sections or 120 triangles
    bool firstEnd = TRUE;
    deleteVertexArray();
    double midR = m_start + m_size / 2.0;
    double si = sin(midR);
    double co = cos(midR);
    double xoffs = _renderer.getRadiusGap() / 2.0 * si;
    double zoffs = _renderer.getRadiusGap() / 2.0 * co;    // move cake pieces outward slightly
    float y = (m_inner) / 4.0f;
    m_treePos.x = m_inner * si;               // position used to render text
    m_treePos.y = y + 0.1f;
    m_treePos.z = m_inner * co;
    float yh = 0.1f;
    Vector nt(0.0f, -1.0f, 0.0f);   // normal top
    for (double i = m_start; i <= end; ) {
        si = std::sin(i);
        co =  std::cos(i);
        Vector nsi(si, -0.4f, co);       // normal side inner
        Vector nso(si, -0.4f, -co);      // normal side outer
        Position p0(xoffs + m_inner * si, y, zoffs + m_inner * co);
        Position p1(xoffs + outer * si, y, zoffs + outer * co);
        Position p2(xoffs + m_inner * si, y - yh, zoffs + m_inner * co);
        Position p3(xoffs + outer * si, y - yh, zoffs + outer * co);

        Color c = _renderer.pos2Color(midR, node->getStage(), m_load, node->isPrimary());
        addPoint(&p0, &c, &nt);
        addPoint(&p1, &c, &nt);
        addPoint(&p0, &c, &nsi);
        addPoint(&p1, &c, &nso);
        addPoint(&p2, &c, &nsi);
        addPoint(&p3, &c, &nso);
        if (idx > 0) {
            addIndex(idx - 0, idx - 5, idx - 6);   // top left triangle
            addIndex(idx - 0, idx + 1, idx - 5);   // top right triangle
            addIndex(idx + 2, idx - 2, idx + 4);   // inner bottom triangle
            addIndex(idx + 2, idx - 4, idx - 2);   // inner top triangle
            addIndex(idx + 3, idx + 5, idx - 1);   // outer bottom triangle
            addIndex(idx - 1, idx - 3, idx + 3);   // outer top triangle
            if (!firstEnd) {
                addIndex(idx + 4, idx + 3, idx + 2);   // end cap top
                addIndex(idx + 4, idx + 5, idx + 3);   // end cap bottom
            }
        }
        else {
            addIndex(idx + 4, idx + 2, idx + 3);   // start cap top
            addIndex(idx + 4, idx + 3, idx + 5);   // start cap bottom
        }

        idx += 6;
        double next = end - i;
        if (step < next) {
            i += step;
        }
        else {
            if (firstEnd) {
                i += next;     // match exact end (needed to draw small triangles !)
                firstEnd = false;
            }
            else {
                break;          // for not looping forever
            }
        }
    }
    create_vao();
}

bool
SunDiscGeometry2::match(const psc::mem::active_ptr<TreeGeometry2>& treeGeo)
{
    bool ret = false;
    if(auto sundisc = psc::mem::dynamic_pointer_cast<SunDiscGeometry2>(treeGeo)) {
        auto lsundisc = sundisc.lease();
        ret = lsundisc->m_inner == m_inner &&
                lsundisc->m_start == m_start &&
                lsundisc->m_size == m_size &&
                lsundisc->m_stage == m_stage &&
                lsundisc->m_load == m_load;
//        if (!ret) {
//            std::cout << "inner " << sundisc->m_inner << "," << m_inner
//                      << " start " << sundisc->m_start << "," << m_start
//                      << " size " << sundisc->m_size << "," << m_size
//                      << " stage " << sundisc->m_stage << "," << m_stage
//                      << " load " << sundisc->m_load << "," << m_load
//                      << " ret " << ret << std::endl;
//        }
    }
    return ret;
}



} /* namespace gl */
} /* namespace psc */