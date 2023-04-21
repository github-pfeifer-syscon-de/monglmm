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


#include "Sensor.hpp"

Sensor::Sensor(Sensors *sensors, std::string _name)
: m_sensors{sensors}
, m_name{_name}
, m_min{1e12}
, m_max{0.0}
{
}

Sensor::~Sensor()
{
}

double
Sensor::getMax() const
{
    return m_max;
}

double
Sensor::getMin() const
{
    return m_min;
}

void
Sensor::adjustMinMax(double value)
{
    if (value > m_max) {
        m_max = value;
    }
    if (value < m_min) {
        m_min = value;
    }
}

std::string
Sensor::getName()
{
    return m_name;
}