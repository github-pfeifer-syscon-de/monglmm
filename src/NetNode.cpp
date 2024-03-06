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

#include <iostream>
#include <StringUtils.hpp>

#include "NetNode.hpp"
#include "NetInfo.hpp"

NetNode::NetNode(const Glib::ustring& name, const Glib::ustring& key)
: psc::gl::NamedTreeNode2(name, key)
{
}

void
NetNode::setConnection(const std::shared_ptr<NetConnection>& conn)
{
    m_conn = conn;
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

bool
NetNode::isMarkGeometryUpdated()
{
    return m_conn
        && m_lastStatus != m_conn->getStatus();
}

void
NetNode::updateMarkGeometry(NaviContext *shaderContext)
{
    m_lastStatus = (m_conn ? m_conn->getStatus() : 0u);
    auto color = getColor();
    Color c(color.get_red(), color.get_green(), color.get_blue());
    auto lgeo = m_geo.lease();
    if (lgeo) {
        lgeo->deleteVertexArray();
        if (m_conn) {
            if (!m_conn->isIncomming()) {
                Position p1{0.0f, 0.07f, 0.0f};
                Position p2{0.0f, -0.03f, 0.0f};
                Position p3{0.07f, 0.02f, 0.0f};
                lgeo->addTri(p1, p2, p3 , c);
            }
            else {
                Position p1{0.07f, 0.07f, 0.0f};
                Position p2{0.0f, 0.02f, 0.0f};
                Position p3{0.07f, -0.03f, 0.0f};
                lgeo->addTri(p1, p2, p3 , c);
            }
        }
        else {
            lgeo->addCube(0.04f, c);
        }
        lgeo->create_vao();
    }
    //geo->setRemoveChildren(false);        // prefere removal with node, and with the nested nodes we otherwise run into trouble
}

void
NetNode::setChildrenTouched(bool touched)
{
    for (auto netchld : m_children) {
        auto netNode = std::dynamic_pointer_cast<NetNode>(netchld.second);
        if (netNode) {
            netNode->setTouched(touched);
        }
        else {
            std::cout << std::source_location::current() << " unexpected typ for NetNode child" << std::endl;
        }
    }
}

void
NetNode::clearUntouched()
{
    for (auto namedchld = m_children.begin(); namedchld != m_children.end(); ) {
        auto chldnode =(*namedchld).second;
        auto netNode = std::dynamic_pointer_cast<NetNode>(chldnode);
        if (netNode && !netNode->isTouched()) {
            //std::cout << std::source_location::current()
            //          << " name " << chldnode->getDisplayName()
            //          << " remove " << std::hex << m_geo.get() << std::dec
            //          << " parent " << std::hex << m_parent << std::dec
            //          << std::endl;
            namedchld = m_children.erase(namedchld);
        }
        else {
            ++namedchld;
        }
    }
}
