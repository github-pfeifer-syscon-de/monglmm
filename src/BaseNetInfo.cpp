/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf 
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

#include <Log.hpp>
#include <unordered_set>
#include <map>
#include <netdb.h>      // getservent_r
#include <arpa/inet.h>  // htons

#include "BaseNetInfo.hpp"


// allow cached resolve for service names...
void
BaseNetInfo::setServiceName(pNetConnect& netConn)
{
    if (netConn->getServiceName().empty()) {
        auto port = netConn->getWellKnownPort();
        std::string serviceName = getServiceName(port);
        netConn->setServiceName(serviceName);
    }
}

void
BaseNetInfo::prepareServiceNames()
{
    // linux specific, shortest method
    struct servent result_buf{};
    struct servent *result{};
    char buf[1024]{};
    while (true) {
        int ret = getservent_r(&result_buf,
                        buf, sizeof(buf),
                        &result);
        if (ret != 0 || result == nullptr) {
            break;
        }
        uint32_t port = ntohs(static_cast<uint16_t>(result->s_port));
        if (strstr(result->s_proto, "tcp") != nullptr) {
            m_portNames.insert(
                    std::pair(
                        port, result->s_name));
        }
    }
}

std::string
BaseNetInfo::getServiceName(uint32_t port)
{
    std::string serviceName;
    if (m_portNames.empty()) {  // as i had no luck to use the system functions...
        prepareServiceNames();
    }
    auto entry = m_portNames.find(port);
    if (entry != m_portNames.end()) {
        serviceName = (*entry).second;
    }
    else {  // show as number
        serviceName = Glib::ustring::sprintf("%d", port);
    }
    return serviceName;
}

void
BaseNetInfo::read(const std::string& name, std::map<std::string, pNetConnect>& netConnections, gint64 now)
{
    std::unordered_set<uint32_t> listeningConn;
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
                auto conn = std::make_shared<NetConnection>(parts, now);
                if (conn->getStatus() == BPF_TCP_LISTEN) {
                    listeningConn.insert(conn->getLocalPort());
                }
                else if (conn->isValid()) {
                    auto entry = netConnections.find(conn->getKey());
                    if (entry == netConnections.end()) {
                        auto lsnEntry = listeningConn.find(conn->getLocalPort());
                        if (lsnEntry != listeningConn.end()) {
                            conn->setIncomming(true);
                        }
                        setServiceName(conn);
                        conn->setTouched(true);
                        netConnections.insert(std::make_pair(conn->getKey(), std::move(conn)));
                    }
                    else {  // use existing
                        auto& con  = (*entry).second;
                        con->setTouched(true);
                        con->setStatus(conn->getStatus());  // use "new" status, other fields are changed as well but at the moment we do not use them
                    }
                }
            }
            else {
                // this will we true for header
                //std::cout << "BaseNetInfo::netConnInfo unused line " << buf << std::endl;
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        if (errno != EAGAIN) {  //we get no eof as this no "normal" file...
            //std::ostringstream oss1;
            //oss1 << "Could not open " << name << " "  << errno
            //     << " " << strerror(errno)
            //     << " ecode " << e.code();
            //Log.addLog(Level.INFO, oss1);
            // e.code() == std::io_errc::stream doesn't help either
        }
    }
    if (stat.is_open()) {
        stat.close();
    }
    //psc::log::Log::logAdd(psc::log::Level::Debug, Glib::ustring::sprintf("accumulated conn %d", netConnections.size()));
}

void
BaseNetInfo::updateConnections(std::vector<pNetConnect>& connections)
{
    gint64 now = g_get_monotonic_time();
    std::map<std::string, pNetConnect> map; // use map to identify existing entries easy
    for (auto& conn : connections) {
        conn->setTouched(false);
        map.insert(std::make_pair(conn->getKey(), std::move(conn)));
    }
    read(getBasePath() + "/tcp", map, now);
    read(getBasePath() + "/tcp6", map, now);
    connections.clear();    // rebuild list from map
    connections.reserve(map.size());    // that may be a bit too much, but more efficent than default
    for (auto iter = map.begin(); iter != map.end(); ++iter) {
        auto& conn = iter->second;
        if (conn->isTouched()) {
            connections.emplace_back(std::move(conn));
        }
        // otherwise just skip
    }
    NetConnection::cleanAddressCache(now);
    //psc::log::Log::logAdd(psc::log::Level::Debug, Glib::ustring::sprintf("after remove %d", connections.size()));
}

