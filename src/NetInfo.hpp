/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2023 RPf 
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
#include <Font2.hpp>

#include "NetConnection.hpp"
#include "NetNode.hpp"
#include "BaseNetInfo.hpp"

class NetInfo
: public BaseNetInfo
{
public:
    NetInfo() = default;
    explicit NetInfo(const NetInfo& orig) = delete;
    virtual ~NetInfo() = default;

    psc::gl::aptrGeom2 draw(NaviContext *pGraph_shaderContext
                , TextContext *_txtCtx, const psc::gl::ptrFont2& pFont
                , bool showNetInfo);
    static constexpr auto NODE_INDENT = 0.2f;
    static constexpr auto NODE_LINESPACE = -0.2f;
    void update();

protected:
    std::map<std::string, std::vector<pNetConnect>>
        group(const std::vector<pNetConnect>& list, uint32_t index);
    void handle(const std::shared_ptr<NetNode>& node,
            const std::vector<pNetConnect>& list,
            uint32_t index);
    std::shared_ptr<NetNode> createNode(
        const std::shared_ptr<NetConnection>& firstEntry
        ,const std::shared_ptr<NetNode>& node
        ,uint32_t index);
    std::string getBasePath() override;
    std::vector<pNetConnect> m_netConnections;

private:
    std::shared_ptr<NetNode> m_root;

};


