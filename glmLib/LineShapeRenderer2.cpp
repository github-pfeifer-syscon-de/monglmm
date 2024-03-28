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


#include "LineShapeRenderer2.hpp"
#include "LineShapeGeometry2.hpp"
#include "TextContext.hpp"

namespace psc {
namespace gl {


LineShapeRenderer2::LineShapeRenderer2()
: shapeStep{0.4}    // shape depth
, shapeGap{0.25}    // gap between shapes forward , side gap will be half of this
{
}


psc::mem::active_ptr<TreeGeometry2>
LineShapeRenderer2::create(const std::shared_ptr<TreeNode2>& _root, NaviContext *_ctx, TextContext *textCtx, const psc::gl::ptrFont2& font)
{
    calculateLeafCount(_root, 1.0, shapeStep);

    Position start(0.0f);
    double width = 10.0;
    setFullWidth(width);
    double size = width / _root->getLeafCount();
    //std::cout << "-------" << std::endl;
    createLineShape(_root, start, width, size, _ctx, textCtx, font, 0);

    return _root->getTreeGeometry();
}

/**
 * @Param node actual tree node
 * @Param start start of process
 * @Param width allocated (according to children)
 * @Param size used (fixed for tree)
 * @Param idx coord index of geometry
 * @Param geo geometry to create
 */
void
LineShapeRenderer2::createLineShape(const std::shared_ptr<TreeNode2>& node, Position& start, double width, double size, NaviContext *_ctx, TextContext *textCtx, const ptrFont2& font, int depth)
{
    auto parentNode = node->getParent();
    bool toUpdate = false;
    auto tempGeo = psc::mem::make_active<LineShapeGeometry2>(font, _ctx);
    auto nodeGeo = psc::mem::dynamic_pointer_cast<LineShapeGeometry2>(node->getTreeGeometry());
    if (!nodeGeo) {
        toUpdate = true;
        nodeGeo = tempGeo;
        node->setTreeGeometry(nodeGeo);
    }
    else {
        auto ltempGeo = tempGeo.lease();
        if (ltempGeo) {
            ltempGeo->update(start, width, size, node, depth);
            toUpdate = !ltempGeo->match(nodeGeo);
        }
    }
    //if (isDebug(node)) {
    //    std::cout << "LineShapeRenderer2::createLineShape"
    //              << " treeGeometry " << (node->getTreeGeometry() ? "y" : "n")
    //              << " nodeGeo " << (nodeGeo ? "y" : "n")
    //              << " toCreate " << (toUpdate ? "y" : "n")
    //              << " toAdd " << (toAdd ? "y" : "n")
    //              << std::endl;
    //}
    auto lnodeGeo = nodeGeo.lease();
    if (lnodeGeo
     && toUpdate) {
        lnodeGeo->update(start, width, size, node, depth);
        lnodeGeo->create(node, *this);
        lnodeGeo->createText(node, _ctx, textCtx);         // this will show text right from start if geometry changes
        if (!parentNode) {
            lnodeGeo->setPosition(start);
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
    Vector forward = Vector(0.0f, 0.0f, shapeStep);
    Position childPos(start + forward);

    for (auto& childNode : children) {
        double cwidth = getSingleSize(width, node, childNode);
        createLineShape(childNode, childPos, cwidth, size, _ctx, textCtx, font, depth + 1);
        childPos.x += cwidth;
    }

}

// distribute space between children by number end nodes
double
LineShapeRenderer2::getSingleSize(double size, const std::shared_ptr<TreeNode2>& node, const std::shared_ptr<TreeNode2>& childNode)
{
    double ratio = childNode->getLeafCount() / node->getLeafCount();
    return size * ratio;
}

bool
LineShapeRenderer2::isDebug(const std::shared_ptr<TreeNode2>& node)
{
    return "xfce4-session" == node->getDisplayName();
}


double
LineShapeRenderer2::calculateLeafCount(const std::shared_ptr<TreeNode2>& node, double dist, double distInc)
{
    double leafCount = 0.0;
    auto& children = node->getChildren();
    if (children.size() > 0) {
        for (auto& chldNode : children) {
            leafCount += calculateLeafCount(chldNode, dist + distInc, distInc);
        }
    }
    else {
        leafCount += 1.0;  // end node just use count
    }
    //if (isDebug(node)) {
    //    std::cout << "LineShapeRenderer2::calculateLeafCount"
    //              << " dist " << dist
    //              << " distInc " << distInc
    //              << " leafCount " << leafCount
    //              << std::endl;
    //}
    node->setLeafCount(leafCount);
    return leafCount;
}

double
LineShapeRenderer2::getDistIncrement()
{
    return shapeStep;
}

const double
LineShapeRenderer2::getShapeForwardGap()
{
    return shapeGap;
}

const double
LineShapeRenderer2::getShapeSideGap()
{
    return shapeGap / 3.0;
}


} /* namespace gl */
} /* namespace psc */