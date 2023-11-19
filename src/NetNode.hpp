/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include <list>
#include <memory>
#include <glibmm.h>
#include <NaviContext.hpp>
#include <TextContext.hpp>
#include <Text.hpp>
#include <linux/bpf.h>  // only accessible version i could find for tcp status

#include "NetConnection.hpp"

class NetNode
{
public:
    NetNode(const Glib::ustring& name, const Glib::ustring& key);
    NetNode(const Glib::ustring& name, const Glib::ustring& key, bool showConn);
    explicit NetNode(const NetNode& orig) = delete;
    virtual ~NetNode();

    void setConnection(const std::shared_ptr<NetConnection>& conn);
    Glib::ustring getDisplayName() const;
    Glib::ustring getKey() const;
    void setTouched(bool touched);
    bool isTouched() const;
    std::list<std::shared_ptr<NetNode>>& getChildren();
    std::shared_ptr<Geometry> getTreeGeometry(NaviContext *shaderContext,
                TextContext *txtCtx,
                Font *pFont);
    void clearUntouched();
    Gdk::RGBA getColor() const;
    void createLineDown(NaviContext *shaderContext, float y);

protected:
    std::list<std::shared_ptr<NetNode>> m_children;
    std::shared_ptr<Geometry> m_geo;
    std::shared_ptr<Text> m_text;
    std::shared_ptr<Geometry> getMarkGeometry(NaviContext *shaderContext);
    std::shared_ptr<Geometry> m_lineDown;
    std::shared_ptr<Geometry> m_lineLeft;
    static constexpr auto LINE_COLOR = 0.8f;
private:
    std::shared_ptr<NetConnection> m_conn;
    Glib::ustring  m_name;
    Glib::ustring  m_key;
    bool m_touched{true};
    uint32_t m_lastStatus{0u};
    float m_lastY{0.0f};
    bool m_showConn;
};



