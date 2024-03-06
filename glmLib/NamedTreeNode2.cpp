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

#include <iostream>
#include <StringUtils.hpp>

#include "NamedTreeNode2.hpp"

namespace psc {
namespace gl {



NamedTreeNode2::NamedTreeNode2(const Glib::ustring& name, const Glib::ustring& key)
: m_name{name}
, m_key{key}
{
}


NamedTreeNode2::~NamedTreeNode2()
{
    //std::cout << std::scouce_location::current() << m_name << std::endl;
    // order matters cleanup bottom to top!
    m_geo.reset();      // clear from other collections as well
    m_children.clear();
    //if (m_geo) {
    //    delete m_geo;   // will remove from depended (parent) as well
    //    m_geo = nullptr;
    //}
}

Glib::ustring
NamedTreeNode2::getDisplayName() const
{
    return m_name;
}

Glib::ustring
NamedTreeNode2::getKey() const
{
    return m_key;
}

ptrNamedTreeNode
NamedTreeNode2::getChild(const Glib::ustring& key)
{
    ptrNamedTreeNode node;
    auto entry = m_children.find(key);
    if (entry != m_children.end()) {
        node = (*entry).second;
    }
    return node;
}

void
NamedTreeNode2::add(const ptrNamedTreeNode& node)
{
    node->setParent(this);
    m_children.insert(std::pair(node->getKey(), node));
}

void
NamedTreeNode2::setParent(NamedTreeNode2* parent)
{
    m_parent = parent;
}

std::list<ptrNamedTreeNode>
NamedTreeNode2::getChildren()
{
    std::list<ptrNamedTreeNode> ret;
    for (auto netchld : m_children) {
        ret.push_back(netchld.second);
    }
    return ret;
}

aptrGeom2
NamedTreeNode2::getGeo()
{
    return m_geo;
}


void
NamedTreeNode2::createLineDown(NaviContext *shaderContext, float y)
{
    if (m_children.size() == 0) {
        if (m_lineDown) {
            m_lineDown.reset();
            m_lastY = 0.0f;
        }
    }
    else {
        if (!m_lineDown
          ||m_lastY != y)  {
            if (m_lineDown) {
                m_lineDown.reset();
            }
            m_lineDown = psc::mem::make_active<Geom2>(GL_LINES, shaderContext);
            auto llineDown = m_lineDown.lease();
            if (llineDown) {
                Position p1{0.0f, 0.0f, 0.0f};
                Position p2{0.0f, y, 0.0f};
                Color c(LINE_COLOR, LINE_COLOR, LINE_COLOR);
                llineDown->addLine(p1, p2, c);
                llineDown->create_vao();
            }
            auto lgeo = m_geo.lease();
            if (lgeo) {
                lgeo->addGeometry(m_lineDown);
            }
            m_lastY = y;
        }
    }
}

aptrGeom2
NamedTreeNode2::getTreeGeometry(
                 NaviContext *shaderContext
                ,TextContext *txtCtx
                ,const ptrFont2& pFont
                ,aptrGeom2 pParentGeo)
{
    bool created = false;
    if (!m_geo) {
        m_geo = psc::mem::make_active<Geom2>(GL_TRIANGLES, shaderContext);
        auto text = psc::mem::make_active<Text2>(GL_TRIANGLES, shaderContext, pFont);
        auto ltext = text.lease();
        if (ltext) {
            ltext->setTextContext(txtCtx);
            ltext->setText(getDisplayName());
            ltext->setScale(0.005f);
            Position pt(0.10f, -0.05f, 0.0f);
            ltext->setPosition(pt);
        }
        auto lgeo = m_geo.lease();
        if (lgeo) {
            lgeo->addGeometry(text);
            if (m_parent) {
                auto lineLeft = psc::mem::make_active<Geom2>(GL_LINES, shaderContext);
                auto llineLeft = lineLeft.lease();
                if (llineLeft) {
                    Position p1{0.0f, 0.02f, 0.0f};
                    Position p2{-NamedTreeNode2::NODE_INDENT, 0.02f, 0.0f};
                    Color c(LINE_COLOR, LINE_COLOR, LINE_COLOR);
                    llineLeft->addLine(p1, p2, c);
                    llineLeft->create_vao();
                }
                lgeo->addGeometry(lineLeft);
            }
            if (m_lineDown) {
                lgeo->addGeometry(m_lineDown);
            }
            if (pParentGeo) {
                auto lParent = pParentGeo.lease();
                if (lParent) {
                    lParent->addGeometry(m_geo);
                }
            }
        }
        created = true;
    }
    if (created
     || isMarkGeometryUpdated()) {
        updateMarkGeometry(shaderContext);
    }

    return m_geo;
}

float
NamedTreeNode2::render(NaviContext *shaderContext
                ,TextContext *txtCtx
                ,const ptrFont2& pFont
                ,const aptrGeom2& pParentGeo)
{
    auto treeGeo = getTreeGeometry(shaderContext, txtCtx, pFont, pParentGeo);
    Position pos{NODE_INDENT, 0.0, 0.0};
    float lastY = 0.0f;
    for (auto chld : getChildren()) {
        pos.y += NODE_LINESPACE;
        auto chldGeo = chld->getTreeGeometry(shaderContext, txtCtx, pFont, treeGeo);
        auto lchldGeo = chldGeo.lease();
        if (lchldGeo) {
            lchldGeo->setPosition(pos);
        }
        lastY = pos.y;
        pos.y += chld->render(shaderContext, txtCtx, pFont, treeGeo);
    }
    createLineDown(shaderContext, lastY);
    return pos.y;   // as we start from 0 return the used space aka. length
}




} /* namespace gl */
} /* namespace psc */