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

#include <string>

class Sensors;  // forward decl

class Sensor
{
public:
    Sensor(Sensors *sensors, std::string _name);
    virtual ~Sensor();

    virtual double read() = 0;
    virtual std::string getUnit() = 0;
    virtual std::string getLabel() = 0;

    std::string getName();
    double getMax() const;
    double getMin() const;

protected:
    void adjustMinMax(double value);

    Sensors *m_sensors;
    std::string m_name;
    double m_min;
    double m_max;
private:

};


