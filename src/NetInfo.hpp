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
#include <map>
#include <Font2.hpp>

#include "NetConnection.hpp"
#include "NetNode.hpp"

class NetInfo
{
public:
    NetInfo();
    explicit NetInfo(const NetInfo& orig) = delete;
    virtual ~NetInfo();

    void update();
    psc::gl::aptrGeom2 draw(NaviContext *pGraph_shaderContext
                , TextContext *_txtCtx, const psc::gl::ptrFont2& pFont
                , bool showNetInfo);
    void setServiceName(std::shared_ptr<NetConnection>& netConn);
    static constexpr auto NODE_INDENT = 0.2f;
    static constexpr auto NODE_LINESPACE = -0.2f;

protected:
    std::map<std::string, std::list<std::shared_ptr<NetConnection>>>
        group(const std::list<std::shared_ptr<NetConnection>>& list, uint32_t index);
    void handle(const std::shared_ptr<NetNode>& node,
            const std::list<std::shared_ptr<NetConnection>>& list,
            uint32_t index);
    void read(const char* name, std::list<std::shared_ptr<NetConnection>>& netConnections);
    std::shared_ptr<NetNode> createNode(
        const std::shared_ptr<NetConnection>& firstEntry
        ,const std::shared_ptr<NetNode>& node
        ,uint32_t index);
private:
    std::shared_ptr<NetNode> m_root;
    std::map<uint32_t, std::string> m_portNames;
    std::list<std::shared_ptr<NetConnection>> m_netConnections;
};


