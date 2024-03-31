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
#include <cstdlib>


#include "LineShapeGeometry2.hpp"

namespace psc {
namespace gl {


LineShapeGeometry2::LineShapeGeometry2(const ptrFont2& font, GeometryContext *_ctx)
: TextTreeGeometry2::TextTreeGeometry2(font, _ctx)
{
    setName("LineShapeGeometry2");
}

void
LineShapeGeometry2::update(Position& start, double width, double size, const std::shared_ptr<TreeNode2>& treeNode, int depth)
{
    m_start = start;
    m_width = width;              // allocated x
    m_size = size;                // used size
    m_stage = treeNode->getStage();
    m_load = treeNode->getLoad();
    m_depth = depth;
}

LineShapeGeometry2::~LineShapeGeometry2()
{
    m_line.reset();
}

void
LineShapeGeometry2::create(const std::shared_ptr<TreeNode2>& node, LineShapeRenderer2& _renderer)
{
    double fwdSize = _renderer.getDistIncrement() - _renderer.getShapeForwardGap();
    Vector forward = Vector(0.0f, 0.0f, fwdSize);
    double xSize = m_size;
    if (xSize > _renderer.getShapeSideGap()) {
        xSize -= _renderer.getShapeSideGap();
    }
    deleteVertexArray();
    m_mid = m_start;
    m_mid.x += m_width / 2.0 ;
    Position end(m_mid);
    end.x += xSize;
    m_treePos = m_mid;               // position used to render text
    m_treePos.z -= _renderer.getDistIncrement() / 2.0;
    Vector depthOffs = Vector(0.0f, -0.1f, 0.0f);
    Vector nt(0.0f, -1.0f, 0.0f);   // normal top
    Vector nf(0.0f,  0.0f, 1.0f);    // normal front
    Vector nb(0.0f,  0.0f, -1.0f);    // normal back
    Vector nr(-1.0f, -0.0f, 0.0f);    // normal right
    Vector nl( 1.0f, -0.0f, 0.0f);    // normal left
    Position p0(m_mid);
    Position p1(m_mid + forward);
    m_mid.x += xSize / 2.0;     // for later connecting adjust to middle

    Position p2(end);
    Position p3(end + forward);
    Position p4(p0 + depthOffs);
    Position p5(p1 + depthOffs);
    Position p6(p2 + depthOffs);
    Position p7(p3 + depthOffs);

    Color c0 = _renderer.pos2Color(m_mid.x, node->getStage(), m_load, node->isPrimary());
    addPoint(&p0, &c0, &nt);    // 0 top
    addPoint(&p1, &c0, &nt);    // 1
    addPoint(&p2, &c0, &nt);    // 2
    addPoint(&p3, &c0, &nt);    // 3
    addPoint(&p1, &c0, &nf);    // 4 front
    addPoint(&p3, &c0, &nf);    // 5
    addPoint(&p5, &c0, &nf);    // 6
    addPoint(&p7, &c0, &nf);    // 7
    addPoint(&p0, &c0, &nb);    // 8 back
    addPoint(&p4, &c0, &nb);    // 9
    addPoint(&p2, &c0, &nb);    // 10
    addPoint(&p6, &c0, &nb);    // 11
    addPoint(&p0, &c0, &nr);    // 12 left
    addPoint(&p1, &c0, &nr);    // 13
    addPoint(&p4, &c0, &nr);    // 14
    addPoint(&p5, &c0, &nr);    // 15
    addPoint(&p2, &c0, &nl);    // 16 right
    addPoint(&p3, &c0, &nl);    // 17
    addPoint(&p6, &c0, &nl);    // 18
    addPoint(&p7, &c0, &nl);    // 19

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

    TreeNode2* parent = node->getParent();
    if (parent != nullptr) {
        psc::mem::active_ptr<TreeGeometry2> parentGeom = parent->getTreeGeometry();
        if (auto parentLineShape = dynamic_pointer_cast<LineShapeGeometry2>(parentGeom)) {
            auto lParentLineShape = parentLineShape.lease();
            if (lParentLineShape) {
                Position pl0(lParentLineShape->m_mid);
                pl0 += forward;
                Position pl1(pl0);
                pl1.z += _renderer.getShapeForwardGap() / 2.0;
                Position pl3(m_mid);
                Position pl2(pl3);
                pl2.z -= _renderer.getShapeForwardGap() / 2.0;
                if (!m_line) {
                    m_line = psc::mem::make_active<Geom2>(GL_LINES, m_ctx);
                    addGeometry(m_line);    // display together
                }
                if (auto lline = m_line.lease()) {
                    lline->deleteVertexArray();
                    lline->setMarkable(false);
                    lline->addLine(pl0, pl1, c0, &nt);
                    lline->addLine(pl1, pl2, c0, &nt);
                    lline->addLine(pl2, pl3, c0, &nt);
                    lline->create_vao();
                }
            }
        }
        else {
            std::cout << "casting parent to LineShapeGeometry2 failed!" << std::endl;
        }
    }
}


// check for meanwhile modifications
bool
LineShapeGeometry2::match(const psc::mem::active_ptr<TreeGeometry2>& treeGeo) {
    auto lineShapeGeo = dynamic_pointer_cast<LineShapeGeometry2>(treeGeo);
    auto lLineShapeGeo = lineShapeGeo.lease();
    bool ret = false;
    if (lLineShapeGeo) {
        ret = lLineShapeGeo->m_start == m_start
           && lLineShapeGeo->m_width == m_width
           && lLineShapeGeo->m_size == m_size
           && lLineShapeGeo->m_stage == m_stage
           && lLineShapeGeo->m_load == m_load;
        //if (!ret) {
        //    std::cout << "LineShapeGeometry2::match"
        //              << " start " << lLineShapeGeo->m_start.x << "," << m_start.x
        //              << " " << lLineShapeGeo->m_start.z << "," << m_start.z
        //              << " width " << lLineShapeGeo->m_width << "," << m_width
        //              << " size " << lLineShapeGeo->m_size << "," << m_size
        //              << " stage " << static_cast<std::underlying_type<psc::gl::TreeNodeState>::type>(lLineShapeGeo->m_stage) << "," << static_cast<std::underlying_type<psc::gl::TreeNodeState>::type>(m_stage)
        //              << " load " << lLineShapeGeo->m_load << "," << m_load
        //              << " ret " << (ret ? "y" : "n") << std::endl;
        //}
    }
    else {
        std::cout << "LineShapeGeometry2::match no lineShapeGeo!"
                  << std::endl;
    }
    return ret;
}



} /* namespace gl */
} /* namespace psc */