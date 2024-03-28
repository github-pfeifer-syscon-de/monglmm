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

#include <memory>

#include "TreeNode2.hpp"
#include "Geom2.hpp"



namespace psc {
namespace gl {



class TreeNode;

class TreeGeometry2
: public Geom2
{
public:
    TreeGeometry2(GLenum type, GeometryContext *ctx);
    virtual ~TreeGeometry2() = default;

    virtual bool match(const psc::mem::active_ptr<TreeGeometry2>& treeGeo) = 0;

    virtual void createText(const std::shared_ptr<TreeNode2>& treeNode, NaviContext *_ctx, TextContext *txtCtx) = 0;
    virtual void removeText() = 0;
    virtual void setTextVisible(bool visible) = 0;


protected:
private:

};






} /* namespace gl */
} /* namespace psc */
