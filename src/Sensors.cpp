/*
 * Copyright (C) 2021 rpf
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


#include "Sensors.hpp"

Sensors::Sensors()
{
}


Sensors::~Sensors()
{
    m_sensors.clear();
}

void
Sensors::add(std::shared_ptr<Sensor> sensor)
{
    std::string name = sensor->getName();
    m_sensors.insert(std::pair<std::string, std::shared_ptr<Sensor>>(name, sensor));
}

const std::shared_ptr<Sensor>
Sensors::getSensor(std::string name) const
{
    auto p = m_sensors.find(name);
    if (p != m_sensors.end()) {
        return p->second;
    }
    return std::shared_ptr<Sensor>();
}

const std::map<std::string, std::shared_ptr<Sensor>>
Sensors::getSensors() const
{
    return m_sensors;
}