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
#include <sstream>

#include <sensors/sensors.h>

#include "LmSensor.hpp"
#include "LmSensors.hpp"
#include "Monitor.hpp"

LmSensor::LmSensor(Sensors *lmSensors, std::string _name, const sensors_chip_name *_chip,
             const sensors_feature *_feat, const sensors_subfeature *_sub_feat)
: Sensor{lmSensors, _name}
, m_chip{_chip}
, m_feat{_feat}
, m_sub_feat{_sub_feat}
{
}


LmSensor::~LmSensor()
{
}

double
LmSensor::read()
{
    LmSensors *lmSensors = ((LmSensors *)m_sensors);
    if (lmSensors->isInited()) {
        double value;
        int ret = sensors_get_value(m_chip, m_sub_feat->number, &value);
        if (ret == 0) {
            adjustMinMax(value);
            return value;
        }
    }
    return 0.0;
}

const char *
LmSensor::featureType()
{
    const char *featName = "";
    switch (m_feat->type) {
	case SENSORS_FEATURE_IN:
	    featName = "in";    break;
	case SENSORS_FEATURE_FAN:
	    featName = "fan";   break;
	case SENSORS_FEATURE_TEMP:
	    featName = "temp";  break;
	case SENSORS_FEATURE_POWER:
	    featName = "power";  break;
	case SENSORS_FEATURE_ENERGY:
	    featName = "energy";  break;
	case SENSORS_FEATURE_CURR:
	    featName = "current";  break;
	case SENSORS_FEATURE_HUMIDITY:
	    featName = "humidity";  break;
	case SENSORS_FEATURE_MAX_MAIN:
	    featName = "min_max";  break;
	case SENSORS_FEATURE_VID:
	    featName = "vid";  break;
	case SENSORS_FEATURE_INTRUSION:
	    featName = "intrusion";  break;
	case SENSORS_FEATURE_MAX_OTHER:
	    featName = "max_other";  break;
	case SENSORS_FEATURE_BEEP_ENABLE:
	    featName = "beep_enable";  break;
	case SENSORS_FEATURE_MAX:
	    featName = "max";  break;
	case SENSORS_FEATURE_UNKNOWN:
	    featName = "unknown";  break;
    }
    return featName;
}


std::string
LmSensor::getUnit()
{
    const char *featUnit = "";
    switch (m_feat->type) {
	case SENSORS_FEATURE_IN:
	    featUnit = "V";
            break;
	case SENSORS_FEATURE_FAN:
	    featUnit = "RPM";
            break;
	case SENSORS_FEATURE_TEMP:
	    featUnit = "°C";
            break;
	case SENSORS_FEATURE_POWER:
	    featUnit = "W";
            break;
	case SENSORS_FEATURE_ENERGY:
	    featUnit = "J";
            break;
	case SENSORS_FEATURE_CURR:
	    featUnit = "A";
            break;
	case SENSORS_FEATURE_HUMIDITY:
	    featUnit = "%";
            break;
	case SENSORS_FEATURE_MAX_MAIN:
	    featUnit = "min_max";
            break;
	case SENSORS_FEATURE_VID:
	    featUnit = "vid";
            break;
	case SENSORS_FEATURE_INTRUSION:
	    featUnit = "intrusion";
            break;
	case SENSORS_FEATURE_MAX_OTHER:
	    featUnit = "max_other";
            break;
	case SENSORS_FEATURE_BEEP_ENABLE:
	    featUnit = "beep_enable";
            break;
	case SENSORS_FEATURE_MAX:
	    featUnit = "max";
            break;
	case SENSORS_FEATURE_UNKNOWN:
	    featUnit = "unknown";
            break;
    }
    return std::string(featUnit);
}

std::string
LmSensor::getLabel()
{
    std::ostringstream oss;
    LmSensors *lmSensors = ((LmSensors *)m_sensors);
    //const char *label = sensors_get_label(m_chip, m_feat);
    // longer name is more verbose but is repeating ...
    oss << lmSensors->getName(m_chip, m_feat)
        << " ";
    oss << Monitor::formatScale(read(), getUnit().c_str(), 1000.0);
    return  oss.str();
}
