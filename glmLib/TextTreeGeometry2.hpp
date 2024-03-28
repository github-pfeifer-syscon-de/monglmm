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

#include "TreeGeometry2.hpp"
#include "Text2.hpp"
#include "Geom2.hpp"


namespace psc {
namespace gl {




class TextTreeGeometry2
: public TreeGeometry2
{
public:
    TextTreeGeometry2(const std::shared_ptr<Font2>& font, GeometryContext *_ctx);
    virtual ~TextTreeGeometry2() = default;

    Position &getTreePos() {
        return m_treePos;
    }
    psc::gl::aptrText2 getText() {
        return m_text;
    }
    void setText(const psc::gl::aptrText2& _text) {
        m_text = _text;
    }
    void createText(const std::shared_ptr<TreeNode2>& treeNode, NaviContext *_ctx, TextContext *txtCtx) override;
    void removeText() override;
    void setTextVisible(bool visible) override;

protected:
    psc::gl::ptrFont2 m_font;
    psc::gl::aptrText2 m_text;
    Position m_treePos;     // text pos

private:

};



} /* namespace gl */
} /* namespace psc */
