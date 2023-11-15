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

#include "NetConnection.hpp"
#include "NetNode.hpp"

class NetInfo
{
public:
    NetInfo();
    explicit NetInfo(const NetInfo& orig) = delete;
    virtual ~NetInfo() = default;

    void update(NaviContext *pGraph_shaderContext,
                TextContext *_txtCtx, Font *pFont, Matrix &persView);
    void setServiceName(std::shared_ptr<NetConnection>& netConn);

protected:
    void buildTree();
    std::map<std::string, std::list<std::shared_ptr<NetConnection>>>
        group(const std::list<std::shared_ptr<NetConnection>>& list, uint32_t index);
    void handle(const std::shared_ptr<NetNode>& node,
            const std::list<std::shared_ptr<NetConnection>>& list,
            uint32_t index);
    double render(NaviContext *shaderContext, TextContext *txtCtx, Font *pFont,  const std::shared_ptr<NetNode>& node);
    void read(const char* name, std::list<std::shared_ptr<NetConnection>>& netConnections);

private:
    std::shared_ptr<NetNode> m_root;
    std::map<uint32_t, std::string> m_portNames;
};


