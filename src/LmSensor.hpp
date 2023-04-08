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

#include "config.h"
#include "Sensor.hpp"

struct sensors_chip_name;
struct sensors_feature;
struct sensors_subfeature;
class Sensors;

class LmSensor : public Sensor
{
public:
    LmSensor(Sensors *lmSensors, std::string _name, const sensors_chip_name *_chip,
             const sensors_feature *_feat, const sensors_subfeature *_sub_feat);
    virtual ~LmSensor();

    double read() override;
    std::string getUnit() override;
    std::string getLabel() override;
    const char * featureType();
    std::string getChipName();

private:

    const sensors_chip_name *m_chip;
    const sensors_feature *m_feat;
    const sensors_subfeature *m_sub_feat;

};
