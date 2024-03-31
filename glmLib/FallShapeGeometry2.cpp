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

#include "FallShapeGeometry2.hpp"

namespace psc {
namespace gl {



FallShapeGeometry2::FallShapeGeometry2(const psc::gl::ptrFont2& font, GeometryContext *_ctx)
: TextTreeGeometry2::TextTreeGeometry2(font, _ctx)
{
    setRemoveChildren(false);   // as we keep track of children in our structure (and need them to keep transform) do not delete them
}

void
FallShapeGeometry2::update(Position& start, double size, const std::shared_ptr<TreeNode2>& treeNode, int depth)
{
    m_start = start;
    m_size = size;              // x
    m_stage = treeNode->getStage();
    m_load = treeNode->getLoad();
    m_depth = depth;
}

void
FallShapeGeometry2::create(const std::shared_ptr<TreeNode2>& node, FallShapeRenderer2& _renderer) {
    double fwdSize = _renderer.getDistIncrement() - _renderer.getShapeForwardGap();
    Vector forward = (!_renderer.isFallDepth(m_depth))
                        ? Vector(0.0f, 0.0f, fwdSize)
                        : Vector(0.0f, -fwdSize, 0.0f);
    double xSize = m_size;
    if (xSize > _renderer.getShapeSideGap()) {
        xSize -= _renderer.getShapeSideGap();
    }
    Position end(m_start);
    end.x += xSize;
    deleteVertexArray();
    double midR = m_start.x + xSize / 2.0;
    m_treePos.x = midR;               // position used to render text
    m_treePos.y = m_start.y;
    m_treePos.z = m_start.z + _renderer.getDistIncrement() / 2.0;
    Vector depthOffs = (!_renderer.isFallDepth(m_depth))
                        ? Vector(0.0f, -0.1f, 0.0f)
                        : Vector(0.0f, 0.0f, -0.1f);
    Vector nt(0.0f, (!_renderer.isFallDepth(m_depth)) ? -1.0f : 0.0f, (!_renderer.isFallDepth(m_depth)) ?  0.0f : 1.0f);   // normal top
    Vector nf(0.0f, (!_renderer.isFallDepth(m_depth)) ?  0.0f : 1.0f, (!_renderer.isFallDepth(m_depth)) ?  1.0f : 0.0f);    // normal front
    Vector nb(0.0f, (!_renderer.isFallDepth(m_depth)) ?  0.0f : -1.0f, (!_renderer.isFallDepth(m_depth)) ? -1.0f : 0.0f);    // normal back
    Vector nr(-1.0f, -0.0f, 0.0f);    // normal right
    Vector nl( 1.0f, -0.0f, 0.0f);    // normal left
    Position p0(m_start);
    Position p1(m_start + forward);
    Position p2(end);
    Position p3(end + forward);
    Position p4(p0 + depthOffs);
    Position p5(p1 + depthOffs);
    Position p6(p2 + depthOffs);
    Position p7(p3 + depthOffs);

    Color c0 = _renderer.pos2Color(m_start.x, node->getStage(), m_load, node->isPrimary());
    Color c2 = _renderer.pos2Color(end.x, node->getStage(), m_load, node->isPrimary());
    addPoint(&p0, &c0, &nt);    // 0 top
    addPoint(&p1, &c0, &nt);    // 1
    addPoint(&p2, &c2, &nt);    // 2
    addPoint(&p3, &c2, &nt);    // 3
    addPoint(&p1, &c0, &nf);    // 4 front
    addPoint(&p3, &c0, &nf);    // 5
    addPoint(&p5, &c2, &nf);    // 6
    addPoint(&p7, &c2, &nf);    // 7
    addPoint(&p0, &c0, &nb);    // 8 back
    addPoint(&p4, &c0, &nb);    // 9
    addPoint(&p2, &c2, &nb);    // 10
    addPoint(&p6, &c2, &nb);    // 11
    addPoint(&p0, &c0, &nr);    // 12 right
    addPoint(&p1, &c0, &nr);    // 13
    addPoint(&p4, &c0, &nr);    // 14
    addPoint(&p5, &c0, &nr);    // 15
    addPoint(&p2, &c2, &nl);    // 16 right
    addPoint(&p3, &c2, &nl);    // 17
    addPoint(&p6, &c2, &nl);    // 18
    addPoint(&p7, &c2, &nl);    // 19


    addIndex(1, 3, 0);   // top near triangle
    addIndex(3, 2, 0);   // top far triangle
    addIndex(6, 7, 5);   // front low triangle
    addIndex(6, 5, 4);   // front upper triangle
    addIndex(9, 11, 10); // back low triangle
    addIndex(9, 10, 8);  // back upper triangle
    addIndex(15, 13, 12); // right low triangle
    addIndex(15, 12, 14); // right upper triangle
    addIndex(17, 19, 18); // left low triangle
    addIndex(17, 18, 16); // left upper triangle
    create_vao();
}

bool
FallShapeGeometry2::match(const psc::mem::active_ptr<TreeGeometry2>& treeGeo) {
    bool ret = false;
    if(auto fallShape = psc::mem::dynamic_pointer_cast<FallShapeGeometry2>(treeGeo)) {
        auto lfallShape = fallShape.lease();
        if (lfallShape) {
            ret = lfallShape->m_start == m_start &&
                  lfallShape->m_size == m_size &&
                  lfallShape->m_stage == m_stage &&
                  lfallShape->m_load == m_load;
        }
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