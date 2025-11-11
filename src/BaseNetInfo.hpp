/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <linux/bpf.h>  // only accessible version i could find for tcp status
#include <StringUtils.hpp>

#include "NetConnection.hpp"


class BaseNetInfo
{
public:
    BaseNetInfo() = default;
    explicit BaseNetInfo(const BaseNetInfo& orig) = delete;
    virtual ~BaseNetInfo() = default;
    void updateConnections(std::vector<pNetConnect>& connections);
    std::string getServiceName(uint32_t port);
protected:
    void setServiceName(std::shared_ptr<NetConnection>& netConn);
    void read(const std::string& name, std::map<std::string, pNetConnect>& netConnections, gint64 now);

    std::map<uint32_t, std::string> m_portNames;
    virtual std::string getBasePath() = 0;
private:
    void prepareServiceNames();

};

