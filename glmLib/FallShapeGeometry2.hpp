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

#include "TextTreeGeometry2.hpp"
#include "FallShapeRenderer2.hpp"


namespace psc {
namespace gl {




class FallShapeGeometry2
: public TextTreeGeometry2
{
public:
    FallShapeGeometry2(const psc::gl::ptrFont2& font, GeometryContext *_ctx);
    virtual ~FallShapeGeometry2() = default;

    void create(const std::shared_ptr<TreeNode2>& node, FallShapeRenderer2 &_renderer);
    bool match(const psc::mem::active_ptr<TreeGeometry2>& treeGeom) override;
    void update(Position& start, double size, const std::shared_ptr<TreeNode2>& treeNode, int depth);

protected:
    Position m_start{0.0f};
    double m_size{0.0};
    TreeNodeState m_stage{TreeNodeState::New};
    float m_load{0.0f};
    int m_depth{0};
private:

};



} /* namespace gl */
} /* namespace psc */