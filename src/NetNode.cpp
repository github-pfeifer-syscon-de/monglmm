/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2023 RPf <gpl3@pfeifer-syscon.de>
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



#include "NetNode.hpp"

NetNode::NetNode(const Glib::ustring& name, const Glib::ustring& key)
: m_name{name}
, m_key{key}
{
}

NetNode::~NetNode()
{
    m_children.clear();
}

void
NetNode::setConnection(const std::shared_ptr<NetConnection>& conn)
{
    m_conn = conn;
}

Glib::ustring
NetNode::getDisplayName() const
{
    return m_name;
}

Glib::ustring
NetNode::getKey() const
{
    return m_key;
}

Gdk::RGBA
NetNode::getColor() const
{
    if (m_conn) {
        switch (m_conn->getStatus())
        {
        case BPF_TCP_ESTABLISHED:
            return Gdk::RGBA("#40e050"); // green
        //?case BPF_TCP_TIME_WAIT: // happens regular (http timeout ?)
        case BPF_TCP_CLOSE:
        case BPF_TCP_CLOSE_WAIT:
        case BPF_TCP_CLOSING:
            return Gdk::RGBA("#e04050"); // red
        default:
            break;
        }
    }
    return Gdk::RGBA("#c0c0c0");    // gray
}

void
NetNode::setTouched(bool _touched)
{
    m_touched = _touched;
}

bool
NetNode::isTouched() const
{
    return m_touched;
}

std::list<std::shared_ptr<NetNode>>&
NetNode::getChildren()
{
    return m_children;
}

std::shared_ptr<Geometry>
NetNode::getMarkGeometry(NaviContext *shaderContext)
{
    m_lastStatus = (m_conn ? m_conn->getStatus() : 0u);
    auto color = getColor();
    auto geo = std::make_shared<Geometry>(GL_TRIANGLES, shaderContext);
    Color c(color.get_red(), color.get_green(), color.get_blue());
    if (m_conn) {
        if (!m_conn->isIncomming()) {
            Position p1{0.0f, 0.07f, 0.0f};
            Position p2{0.0f, -0.03f, 0.0f};
            Position p3{0.07f, 0.02f, 0.0f};
            geo->addTri(p1, p2, p3 , c);
        }
        else {
            Position p1{0.07f, 0.07f, 0.0f};
            Position p2{0.0f, 0.02f, 0.0f};
            Position p3{0.07f, -0.03f, 0.0f};
            geo->addTri(p1, p2, p3 , c);
        }
    }
    else {
        geo->addCube(0.04f, c);
    }
    geo->create_vao();
    if (m_text) {
        geo->addGeometry(m_text.get());
    }
    geo->setRemoveChildren(false);        // prefere removal with node, and with the nested nodes we otherwise run into trouble

    return geo;
}

std::shared_ptr<Geometry>
NetNode::getTreeGeometry(
                NaviContext *shaderContext,
                TextContext *txtCtx,
                Font *pFont)
{
    if (!m_text) {
        m_text = std::shared_ptr<Text>(txtCtx->createText(shaderContext, pFont));
        m_text->setText(getDisplayName());
        m_text->setScale(0.005f);
        Position pt(0.10f, -0.05f, 0.0f);
        m_text->setPosition(pt);
    }
    if (!m_geo
     || (m_conn
         && m_lastStatus != m_conn->getStatus())) {
        m_geo = getMarkGeometry(shaderContext);
    }
    // may need special consideration when destructing ...
    //if (auto parent = node->m_parent.lock()) {
    //    parent->getTreeGeometry()->addGeometry(treeGeo);
    //}
    //else {
    //}

    return m_geo;
}


void
NetNode::clearUntouched()
{
    for (auto netchld = m_children.begin(); netchld != m_children.end(); ) {
        if (!(*netchld)->isTouched()) {
            //std::cout << "erasing "
            //          << " name " << (*netchld)->getDisplayName()
            //          << " use " << (*netchld).use_count()
            //          << std::endl;
            netchld = m_children.erase(netchld);
        }
        else {
            ++netchld;
        }
    }
}
