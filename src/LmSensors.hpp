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

#include "config.h"
#include "Sensors.hpp"

class LmSensors : public Sensors
{
public:
    LmSensors();
    virtual ~LmSensors();

    const std::shared_ptr<Sensor> getPrimDefault() override;
    const std::shared_ptr<Sensor> getSecDefault() override;

    bool isInited() const;
    std::string getName(const sensors_chip_name *chip, const sensors_feature *feat);
private:


    int m_init;
};
