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


#include "TreeNode2.hpp"
#include "TextContext.hpp"
#include "TextTreeGeometry2.hpp"

namespace psc {
namespace gl {



TextTreeGeometry2::TextTreeGeometry2(const std::shared_ptr<Font2>& font, GeometryContext *_ctx)
: TreeGeometry2::TreeGeometry2(GL_TRIANGLES, _ctx)
, m_font{font}
{
}

void
TextTreeGeometry2::createText(const std::shared_ptr<TreeNode2>& treeNode, NaviContext *_ctx, TextContext *txtCtx)
{
    if (!m_text) {
        m_text = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, _ctx, m_font);
        auto ltext = m_text.lease();
        if (ltext) {
            ltext->setTextContext(txtCtx);
            Glib::ustring dName = treeNode->getDisplayName();
            ltext->setText(dName);
            ltext->setScale(0.0040f);
            Position p2 = getTreePos();
            ltext->setPosition(p2);
            addGeometry(m_text);   // this will care for transfrom if piece is moved
            ltext->setVisible(treeNode->isSelected());
        }
    }
    // direct display will not use the correct (concat) transform
    //m_text->display(persView);
}

void
TextTreeGeometry2::removeText() {
    m_text.reset();
}


void
TextTreeGeometry2::setTextVisible(bool visible)
{
    if (m_text) {
        auto ltext = m_text.lease();
        if (ltext) {
            ltext->setVisible(visible);
        }
    }
}




} /* namespace gl */
} /* namespace psc */