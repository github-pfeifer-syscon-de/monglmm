/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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
                            m_portNames.insert(std::make_pair(porti, name));
                        }
                    }
                }
            }
        }
    }
    catch (const std::ios_base::failure& e) {
        //std::ostringstream oss1;
        //oss1 << "Could not open /etc/services " << errno
        //     << " " << strerror(errno)
        //     << " ecode " << e.what();
        // e.code() == std::io_errc::stream doesn't help either
    }
    if (stat.is_open()) {
       stat.close();
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
                        con->setStatus(conn->getStatus());  // use "new" status
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
            std::ostringstream oss1;
            oss1 << "Could not open " << name << " "  << errno
                 << " " << strerror(errno)
                 << " ecode " << e.code();
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
        auto& conn = (*iter).second;
        if (conn->isTouched()) {
            connections.emplace_back(std::move(conn));
        }
        // otherwise just skip
    }
    NetConnection::cleanAddressCache(now);
    //psc::log::Log::logAdd(psc::log::Level::Debug, Glib::ustring::sprintf("after remove %d", connections.size()));
}

