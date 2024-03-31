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
#include <iostream>


#include "FallShapeRenderer2.hpp"
#include "FallShapeGeometry2.hpp"
#include "TextContext.hpp"


namespace psc {
namespace gl {


FallShapeRenderer2::FallShapeRenderer2()
: shapeStep{0.4}    // shape depth
, shapeGap{0.15}    // gap between shapes forward , side gap will be half of this
, m_depth_fall{5}   // where to fall off ~ user proc
{
}


psc::mem::active_ptr<psc::gl::TreeGeometry2>
FallShapeRenderer2::create(const std::shared_ptr<TreeNode2>& _root, NaviContext *_ctx, TextContext *textCtx, const psc::gl::ptrFont2& font)
{
    calculateLeafCount(_root, 1.0, shapeStep);

    Position start(0.0f);
    double width = 10.0;
    setFullWidth(width);
    //std::cout << "-------" << std::endl;
    createFallShape(_root, start, width, _ctx, textCtx, font, 0);

    return _root->getTreeGeometry();
}

/**
 * @Param node actual tree node
 * @Param start start of level
 * @Param size x size of level
 * @Param idx coord index of geometry
 * @Param geo geometry to create
 */
void
FallShapeRenderer2::createFallShape(const std::shared_ptr<TreeNode2>& node, Position& start, double size, NaviContext *_ctx, TextContext *textCtx, const psc::gl::ptrFont2& font, int depth) {

    auto parentNode = node->getParent();
    bool toUpdate = false;
    auto tempGeo = psc::mem::make_active<FallShapeGeometry2>(font, _ctx);
    auto nodeGeo = psc::mem::dynamic_pointer_cast<FallShapeGeometry2>(node->getTreeGeometry());
    if (!nodeGeo) {
        toUpdate = true;
        nodeGeo = tempGeo;
        node->setTreeGeometry(nodeGeo);
    }
    else {
        auto ltempGeo = tempGeo.lease();
        if (ltempGeo) {
            ltempGeo->update(start, size, node, depth);
            toUpdate = !ltempGeo->match(nodeGeo);
        }
    }
    auto lnodeGeo = nodeGeo.lease();
    if (lnodeGeo
     && toUpdate) {
        lnodeGeo->update(start, size, node, depth);
        lnodeGeo->create(node, *this);
        lnodeGeo->createText(node, _ctx, textCtx);         // this will show text right from start if geometry changes
        if (!parentNode) {
            //lnodeGeo->setPosition(pos);
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
    Vector forward = (!isFallDepth(depth))
                    ? Vector(0.0f, 0.0f, shapeStep)
                    : Vector(0.0f, -shapeStep, 0.0f);
    Position childPos(start + forward);

    for (auto& childNode : children) {
        double single = getSingleSize(size, node, childNode);
        createFallShape(childNode, childPos, single, _ctx, textCtx, font, depth + 1);
        childPos.x += single;
    }

}

// distribute space between children by number end nodes
double
FallShapeRenderer2::getSingleSize(double size, const std::shared_ptr<TreeNode2>& node, const std::shared_ptr<TreeNode2>& childNode) {
    double ratio = childNode->getLeafCount() / node->getLeafCount();
    if (ratio == 0.0) {
        ratio = 1.0;
    }
    return size * ratio;
}

double
FallShapeRenderer2::calculateLeafCount(const std::shared_ptr<TreeNode2>& node, double dist, double distInc) {
    double leafCount = 0.0;
    auto& children = node->getChildren();
    if (children.size() > 0) {
        for (auto& chldNode : children) {
            leafCount += calculateLeafCount(chldNode, dist + distInc, distInc);
        }
    }
    else {
        leafCount += 1.0;  // just use count
    }
    node->setLeafCount(leafCount);
    return leafCount;
}


} /* namespace gl */
} /* namespace psc */