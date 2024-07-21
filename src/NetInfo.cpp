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
#include <fstream>
#include <StringUtils.hpp>
#include <netdb.h>  // servent
#include <string.h>


#include "NetInfo.hpp"



std::map<std::string, std::list<std::shared_ptr<NetConnection>>>
NetInfo::group(const std::list<std::shared_ptr<NetConnection>>& list, uint32_t index)
{
    std::map<std::string, std::list<std::shared_ptr<NetConnection>>> collected;
    for (auto conn : list) {
        std::vector<Glib::ustring> parts = conn->getRemoteAddr()->getNameSplit();
        if (parts.size() >= index) {
            std::string key = parts[parts.size() - index];
            if (parts.size() == index) {
                key += conn->getGroupSuffix();
            }
            auto entry = collected.find(key);
            if (entry == collected.end()) {
                collected.insert(
                        std::pair(key, std::list<std::shared_ptr<NetConnection>>()));
                entry = collected.find(key);
            }
            (*entry).second.push_back(conn);
        }
    }
    //for (auto entry : collected) {
    //    std::cout << entry.first << " " << entry.second.size() << std::endl;
    //}
    return collected;
}

std::shared_ptr<NetNode>
NetInfo::createNode(
    const std::shared_ptr<NetConnection>& connection
    ,const std::shared_ptr<NetNode>& node
    ,uint32_t index)
{
    auto nameSplit = connection->getRemoteAddr()->getNameSplit();
    Glib::ustring newName = nameSplit[nameSplit.size() - index];
    bool matching = false;
    auto key = newName;
    if (nameSplit.size() == index) {
        newName = Glib::ustring::sprintf("%s:%s", newName, connection->getServiceName());    // , firstEntry->isIncomming() ? '<' : '>'
        key = newName + connection->getGroupSuffix();
        matching = true;
    }
    auto namedNode = node->getChild(key);
    std::shared_ptr<NetNode> newNode = std::dynamic_pointer_cast<NetNode>(namedNode);
    if (!newNode) {
        newNode = std::make_shared<NetNode>(newName, key);
        node->add(newNode);
    }
    newNode->setTouched(true);
    if (matching) { // only these have a usable status
        newNode->setConnection(connection);
    }
    return newNode;
}

void
NetInfo::handle(const std::shared_ptr<NetNode>& node,
            const std::list<std::shared_ptr<NetConnection>>& list,
            uint32_t index)
{
    std::map<std::string, std::list<std::shared_ptr<NetConnection>>> collected = group(list, index);
    node->setChildrenTouched(false);
    for (auto entry : collected) {
        auto key = entry.first;
        if (!key.empty()) {
            std::list<std::shared_ptr<NetConnection>>& connEntries = entry.second;
            for (auto conEntry = connEntries.begin(); conEntry != connEntries.end(); ) {
                auto conn = *conEntry;
                auto addrEntry = conn->getRemoteAddr()->getNameSplit();
                if (addrEntry.size() == index) {    // make node for any finished entry
                    createNode(conn, node, index);
                    conEntry = connEntries.erase(conEntry);
                }
                else {
                    ++conEntry;
                }
            }
            if (connEntries.size() > 0) {  // all non finished keep grouped
                auto newNode = createNode(*(connEntries.begin()), node, index);
                handle(newNode, connEntries, index + 1);
            }
        }
    }
    node->clearUntouched();
}

psc::gl::aptrGeom2
NetInfo::draw(NaviContext *pGraph_shaderContext
            , TextContext *txtCtx, const psc::gl::ptrFont2& pFont
            , bool showNetInfo)
{
    psc::gl::aptrGeom2 treeGeo;
    if (showNetInfo) {
        if (!m_root) {
            m_root = std::make_shared<NetNode>("@", "@");   // u1f310 might be an alternative but is not commonly supported
            auto treeGeo = m_root->getTreeGeometry(pGraph_shaderContext, txtCtx, pFont, nullptr);
            pGraph_shaderContext->addGeometry(treeGeo);     // only add this as main elements to context, children are rendered by default
            auto treeGeoLease = treeGeo.lease();
            if (treeGeoLease) {
                Position pos{1.5f, 2.3f, 0.0f};
                treeGeoLease->setPosition(pos);
            }
        }
        handle(m_root, m_netConnections, 1);
        m_root->render(pGraph_shaderContext, txtCtx, pFont, nullptr);
        treeGeo = m_root->getGeo();
    }
    else {
        if (m_root) {
            treeGeo = m_root->getGeo();
            if (treeGeo) {
                pGraph_shaderContext->removeGeometry(treeGeo.get());
            }
            m_root.reset(); // recreate if needed
        }
    }
    return treeGeo;
}

std::string
NetInfo::getBasePath()
{
    return "/proc/net";
}

void
NetInfo::update()
{
    m_netConnections = updateConnections();
}