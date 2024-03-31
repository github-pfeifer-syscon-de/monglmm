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
#include "SunDiscRenderer2.hpp"

namespace psc {
namespace gl {


SunDiscRenderer2::SunDiscRenderer2()
: radiusStep{0.4}	// disc thickness
, radiusGap{0.15}   // gap between discs outward , disks will be moved radiusGap/2 from origin
{
}

psc::mem::active_ptr<psc::gl::TreeGeometry2>
SunDiscRenderer2::create(
          const std::shared_ptr<TreeNode2>& _root
        , NaviContext* _ctx
        , TextContext* textCtx
        , const psc::gl::ptrFont2& font)
{
    calculateLeafCount(_root, 1.0, radiusStep);

    Position pos(0.0f);
    setFullWidth(M_PI * 2.0);
    //std::cout << "-------" << std::endl;
    createSunDisc(0.0, _root, 0.0, getFullWidth(), _ctx, textCtx, pos, font, 0);

    return _root->getTreeGeometry();
}

/**
 * @Param inner inner radius
 * @Param node actual tree node
 * @Param start rad start of section
 * @Param size rad size of section
 * @Param idx coord index of geometry
 * @Param geo geometry to create
 */
void
SunDiscRenderer2::createSunDisc(
          double inner
        , const std::shared_ptr<TreeNode2>& node
        , double start
        , double size
        , NaviContext* _ctx
        , TextContext* textCtx
        , Position& pos
        , const psc::gl::ptrFont2& font
        , int depth)
{
    auto parentNode = node->getParent();
    bool toUpdate = false;
    auto tempGeo = psc::mem::make_active<SunDiscGeometry2>(font, _ctx);
    auto nodeGeo = psc::mem::dynamic_pointer_cast<SunDiscGeometry2>(node->getTreeGeometry());
    if (!nodeGeo) {
        toUpdate = true;
        nodeGeo = tempGeo;
        node->setTreeGeometry(nodeGeo);
    }
    else {
        auto ltempGeo = tempGeo.lease();
        if (ltempGeo) {
            ltempGeo->update(inner, start, size, pos, node);
            toUpdate = !ltempGeo->match(nodeGeo);
        }
    }
    auto lnodeGeo = nodeGeo.lease();
    if (lnodeGeo
     && toUpdate) {
        lnodeGeo->update(inner, start, size, pos, node);
        lnodeGeo->create(node, *this);
        lnodeGeo->createText(node, _ctx, textCtx);         // this will show text right from start if geometry changes
        if (!parentNode) {
            lnodeGeo->setPosition(pos);
            //_ctx->addGeometry(sGeo);        // required for clicking
        }
    }
    if (parentNode) {        // in any case rebuild structure as parent may have changed
        auto parentGeo = parentNode->getTreeGeometry();
        //std::cout << "add " << std::hex << tGeo << " to " << std::hex << parentGeo << std::endl;
        auto lParentGeo = parentGeo.lease();
        if (lParentGeo) {
            lParentGeo->addGeometry(nodeGeo);       // replicate structure on geometries to allow breaking out tree parts
        }
    }

    auto& children = node->getChildren();
    double st = start;
    for (auto& childNode : children) {
        double single = getSingleSize(size, node, childNode);
        createSunDisc(inner + radiusStep, childNode, st, single, _ctx, textCtx, pos, font, depth + 1);
        st += single;
    }
}

// just distribute space between children evenly (unevenly sized leaves)
//double
//SunDiscRenderer2::getSingleSize(double size, TreeNode *node, TreeNode *childNode) {
//    return size / (double)(node->getChildren().size());
//}

// distribute space between children depending on leaves
//   (evenly sized leaves (see also calculateLeafCount to size leaves similar))
double
SunDiscRenderer2::getSingleSize(double size, const std::shared_ptr<TreeNode2>& node, const std::shared_ptr<TreeNode2>& childNode)
{
    double ratio = childNode->getLeafCount() / node->getLeafCount();
    return size * ratio;
}

double
SunDiscRenderer2::calculateLeafCount(const std::shared_ptr<TreeNode2>& node, double dist, double distInc)
{
    double leafCount = 0.0;
    auto& children = node->getChildren();
    if (children.size() > 0) {
        for (auto& chldNode : children) {
            leafCount += calculateLeafCount(chldNode, dist + distInc, distInc);
        }
    }
    else {
        leafCount += 1.0 / dist;  // scale by distance (distribute size better than by angle) smaller angel outwards but similar arc length ...
    }
    node->setLeafCount(leafCount);
    return leafCount;
}


} /* namespace gl */
} /* namespace psc */