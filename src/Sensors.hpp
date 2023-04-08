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

#pragma once

#include <map>
#include <memory>

#include "Sensor.hpp"

class Sensors
{
public:
    Sensors();
    virtual ~Sensors();

    const std::shared_ptr<Sensor> getSensor(std::string name) const;
    virtual const std::shared_ptr<Sensor> getPrimDefault() = 0;
    virtual const std::shared_ptr<Sensor> getSecDefault() = 0;
    const std::map<std::string, std::shared_ptr<Sensor>> getSensors() const;
protected:
    void add(std::shared_ptr<Sensor> sensor);
private:

    std::map<std::string, std::shared_ptr<Sensor>> m_sensors;
};

