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

#include <map>
#include <memory>
#include <glibmm.h>
#include <linux/bpf.h>  // only accessible version i could find for tcp status
#include <NamedTreeNode2.hpp>

#include "NetConnection.hpp"

class NetNode;

typedef std::shared_ptr<NetNode> ptrNetNode;

class NetNode
: public psc::gl::NamedTreeNode2
{
public:
    NetNode(const Glib::ustring& name, const Glib::ustring& key);
    explicit NetNode(const NetNode& orig) = delete;
    virtual ~NetNode() = default;

    void setConnection(const std::shared_ptr<NetConnection>& conn);
    void setTouched(bool touched);
    bool isTouched() const;
    void setChildrenTouched(bool touched);
    void clearUntouched();
    Gdk::RGBA getColor() const;
    bool isMarkGeometryUpdated() override;
    void updateMarkGeometry(NaviContext *shaderContext) override;
protected:

private:
    std::shared_ptr<NetConnection> m_conn;
    bool m_touched{true};
    uint32_t m_lastStatus{0u};
};

