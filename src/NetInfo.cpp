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

NetInfo::NetInfo()
{
}

NetInfo::~NetInfo()
{
}

// allow cached resolve for service names...
void
NetInfo::setServiceName(std::shared_ptr<NetConnection>& netConn)
{
    if (m_portNames.empty()) {  // as i had no luck to use the system functions...
        std::ifstream stat;
        std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit | std::ios::eofbit;
        stat.exceptions(exceptionMask);
        try {   // alternativ  /usr/share/iana-etc/port-numbers.iana
            stat.open("/etc/services");
            while (!stat.eof()) {
                std::string line;
                std::getline(stat, line);
                if (!line.starts_with('#')
                 && line.length() > 3) {
                    std::vector<Glib::ustring> parts;
                    StringUtils::splitRepeat(line, ' ', parts);
                    if (parts.size() >= 2) {
                        auto name = parts[0];
                        auto portProt = parts[1];
                        auto pos = portProt.find('/');
                        if (pos != portProt.npos) {
                            auto port = portProt.substr(0, pos);
                            auto prot = portProt.substr(pos + 1);
                            //std::cout << __FILE__ << "::setRemoteServiceName"
                            //          << " found " << name
                            //          << " port " << port
                            //          << " prot " << prot << std::endl;
                            if ("tcp" == prot) {
                                uint32_t porti = std::stoi(port);
                                m_portNames.insert(std::pair<uint32_t, std::string>(porti, name));
                            }
                        }
                    }
                }
            }
        }
        catch (const std::ios_base::failure& e) {
            std::ostringstream oss1;
            oss1 << "Could not open /etc/services " << errno
                 << " " << strerror(errno)
                 << " ecode " << e.what();
            // e.code() == std::io_errc::stream doesn't help either
       }
       if (stat.is_open()) {
           stat.close();
       }
    }
//    if (m_remoteSericeName.empty()) {
//        auto serv = Gio::NetworkService::create(
//            "",	"tcp", Glib::ustring::sprintf("%d", m_remotePort));
//        if (!serv->get_service().empty()) {
//            std::cout << "NetConnection::getRemoteServiceName "
//                      << " gionetw. " << m_remotePort
//                      << " to " << serv->get_service() << std::endl;
//            return serv->get_service();
//        }
//    }
//    if (m_portNames.empty()) {
//        struct servent result_buf;
//        struct servent *result;
//        char buf[1024];
//        // getservbyport seems to have been deprecated ... (always returns null)
//        //   from the looks i would expect there is a free required ??? but the examples do not show any sign of this
//        int ret = getservbyport_r(htons(static_cast<uint16_t>(netConn->getRemotePort())), "tcp",
//                                  &result_buf, buf, sizeof(buf), &result);
//        if (ret == 0) {
//            if (strlen(buf) > 0) {
//                netConn.setRemoteSericeName(buf);   // same result_buf.s_name;
//            }
//            //if (result != nullptr) {
//            //    std::cout << __FILE_NAME__ << "::getRemoteServiceName "
//            //              << " name " << result->s_name
//            //              << " proto " << result->s_proto << std::endl;
//            //}
//        }
//        else {
//            std::cout << "errno " << errno << " " << strerror(errno) << std::endl;
//            std::cout << "Checking " << m_remotePort << " servent is null" << std::endl;
//        }
//        if (netConn.getRemoteSericeName().empty()) {   // still empty use number ...
//            netConn.setRemoteSericeName(Glib::ustring::sprintf("%d", m_remotePort));
//        }
//    }
    // this leaves out https ?
//    if (m_portNames.empty()) {
//        struct servent result_buf;
//        struct servent *result;
//        char buf[1024];
//        while (true) {
//            memset(buf, 0, sizeof(buf));
//            memset(&result_buf, 0, sizeof(result_buf));
//            int ret = getservent_r(&result_buf,
//                            buf, sizeof(buf),
//                            &result);
//            if (ret != 0 || result == nullptr) {
//                break;
//            }
//            if (result->s_port >= 440 && result->s_port <= 450) {
//                std::cout << __FILE__ "::setRemoteServiceName"
//                          << "  add port " << result->s_port
//                          << " name " << result->s_name
//                          << " prot " << result->s_proto
//                          << std::endl;
//            }
//            if (strstr(result->s_proto, "tcp") != nullptr) {
//                m_portNames.insert(
//                        std::pair<uint32_t, std::string>(
//                            result->s_port, result->s_name));
//            }
//        }
//        std::cout << __FILE__ "::setRemoteServiceName"
//                  << "entries " << m_portNames.size()
//                  << std::endl;
//    }
    if (netConn->getServiceName().empty()) {
        auto port = netConn->getWellKnownPort();
        auto entry = m_portNames.find(port);
        std::string serviceName;
        if (entry != m_portNames.end()) {
            serviceName = (*entry).second;
        }
        else {  // show in as number
            serviceName = Glib::ustring::sprintf("%d", netConn->getRemotePort());
        }
        netConn->setServiceName(serviceName);
    }
}


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

void
NetInfo::read(const char* name, std::list<std::shared_ptr<NetConnection>>& netConnections)
{
    std::list<std::shared_ptr<NetConnection>> listeningConn;
    std::ifstream stat;
    std::ios_base::iostate exceptionMask = stat.exceptions() | std::ios::failbit | std::ios::badbit | std::ios::eofbit;
    stat.exceptions(exceptionMask);
    try {
        stat.open(name);
        while (!stat.eof()) {
            std::string buf;
            std::getline(stat, buf);
            std::vector<Glib::ustring> parts;
            StringUtils::splitRepeat(buf, ' ', parts);
            if (parts.size() >= 4
             && parts[0] != "sl") { // skip heading
                auto localipPort = parts[1];
                auto remoteipPort = parts[2];
                auto status = parts[3];
                auto conn = std::make_shared<NetConnection>(localipPort, remoteipPort, status);
                if (conn->getStatus() == BPF_TCP_LISTEN) {
                    listeningConn.emplace_back(conn);
                }
                else {
                    auto conn = std::make_shared<NetConnection>(localipPort, remoteipPort, status);
                    if (conn->isValid()) {
                        netConnections.emplace_back(conn);
                        for (auto listen : listeningConn) {
                            if (conn->getLocalPort() == listen->getLocalPort()) {
                                conn->setIncomming(true);
                                break;
                            }
                        }
                        setServiceName(conn);
                    }
                }
            }
            else {
                // this will we true for header
                //std::cout << "StarDraw::netConnInfo unused line " << buf << std::endl;
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (errno != EAGAIN) {  //we get no eof as this no "normal" file...
            std::ostringstream oss1;
            oss1 << "Could not open /proc/net/tcp " << errno
                 << " " << strerror(errno)
                 << " ecode " << e.code();
            // e.code() == std::io_errc::stream doesn't help either
        }
    }
    if (stat.is_open()) {
        stat.close();
    }
}

void
NetInfo::update()
{
    m_netConnections.clear();
    read("/proc/net/tcp", m_netConnections);
    read("/proc/net/tcp6", m_netConnections);
}


psc::gl::aptrGeom2
NetInfo::draw(NaviContext *pGraph_shaderContext,
                TextContext *txtCtx, const psc::gl::ptrFont2& pFont)
{
    if (!m_root) {
        m_root = std::make_shared<NetNode>("@", "@");   // u1f310 might be an alternative but is not commonly supported
        auto treeGeo = m_root->getTreeGeometry(pGraph_shaderContext, txtCtx, pFont, nullptr);
        //pGraph_shaderContext->addGeometry(treeGeo.get()); // only add this as main elements to context, children are rendered by default
        auto treeGeoLease = treeGeo.lease();
        if (treeGeoLease) {
            Position pos{1.5f, 2.3f, 0.0f};
            treeGeoLease->setPosition(pos);
        }
    }
    handle(m_root, m_netConnections, 1);
    m_root->render(pGraph_shaderContext, txtCtx, pFont, nullptr);
    auto treeGeo = m_root->getGeo();
    return treeGeo;
}

