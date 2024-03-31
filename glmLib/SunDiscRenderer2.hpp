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



#include "NaviContext.hpp"
#include "TextContext.hpp"
#include "TreeNode2.hpp"
#include "Geom2.hpp"
#include "Font2.hpp"
#include "TreeRenderer2.hpp"
#include "TreeGeometry2.hpp"
#include "active_ptr.hpp"



namespace psc {
namespace gl {



class SunDiscRenderer2
: public TreeRenderer2
{
public:
    SunDiscRenderer2();
    virtual ~SunDiscRenderer2() = default;

    psc::mem::active_ptr<psc::gl::TreeGeometry2> create(
          const std::shared_ptr<TreeNode2>& _root
        , NaviContext* _ctx
        , TextContext* textCtx
        , const psc::gl::ptrFont2& font) override;
    void createSunDisc(
          double inner
        , const std::shared_ptr<TreeNode2>& node
        , double start
        , double size
        , NaviContext* _ctx
        , TextContext* textCtx
        , Position& pos
        , const psc::gl::ptrFont2& font
        , int depth);
    double getSingleSize(double size, const std::shared_ptr<TreeNode2>& node, const std::shared_ptr<TreeNode2>& childNode);
    double getDistIncrement() override {
        return radiusStep;
    }
    double getRadiusGap() {
        return radiusGap;
    }
protected:
    double calculateLeafCount(const std::shared_ptr<TreeNode2>& node, double dist, double distInc);
    double radiusStep;
    double radiusGap;
private:

};



} /* namespace gl */
} /* namespace psc */