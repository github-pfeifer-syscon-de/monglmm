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
#include <sensors/sensors.h>

#include "LmSensors.hpp"
#include "LmSensor.hpp"

enum INIT {
    NO_INIT,
    TRYED,
    DONE
};

LmSensors::LmSensors()
: Sensors()
, m_init{NO_INIT}
{
    if (m_init == NO_INIT) {
	m_init = TRYED;
	int ret = sensors_init(nullptr);
	if (ret != 0) {
	    fprintf(stderr, "lmsensors initialization failed %d\n", ret);
            return;
	}
	m_init = DONE;
        int i = 0;
	while (1) {
	    const sensors_chip_name *chip = sensors_get_detected_chips(nullptr, &i);
	    if (chip == nullptr) {
                break;
            }
            int j = 0;
            while (1) {
                const sensors_feature *feat = sensors_get_features(chip, &j);
                if (feat == nullptr) {
                    break;
                }
                int k = 0;
                while (1) {
                    const sensors_subfeature *sub_feat = sensors_get_all_subfeatures(chip, feat, &k);
                    if (sub_feat == nullptr) {
                        break;
                    }
                    //fprintf(stdout, "lmsensors_read sub-feat %d %s\n", k, sub_feat->name);
                    if (sub_feat->type == SENSORS_SUBFEATURE_IN_INPUT ||
                        sub_feat->type == SENSORS_SUBFEATURE_FAN_INPUT ||
                        sub_feat->type == SENSORS_SUBFEATURE_TEMP_INPUT ||
                        sub_feat->type == SENSORS_SUBFEATURE_INTRUSION_ALARM) {  // only look for usual types
                        std::string name = getName(chip, feat);
                        std::shared_ptr<LmSensor> sensor = std::make_shared<LmSensor>(this, name, chip, feat, sub_feat);
                        add(sensor);
                    }
                }
	    }
	}
    }
}

LmSensors::~LmSensors()
{
    if (m_init == DONE) {
	sensors_cleanup();
	m_init = NO_INIT;
    }
}

std::string
LmSensors::getName(const sensors_chip_name *chip, const sensors_feature *feat)
{
    if (m_init == DONE) {
        char chip_name[128];
        int ret = sensors_snprintf_chip_name(chip_name, sizeof(chip_name), chip);
        //fprintf(stdout, "lmsensors_read chip %d \"%s\"\n", i, chip_name);
        if (ret > 0) {
            std::string name(chip_name);
            const char *label = sensors_get_label(chip, feat);
            name += std::string(" ") + std::string(label);
            return name;
        }
    }
    return std::string();
}

// This is tailored for my needs other may need to look into config
//    but as pentiums are not that uncommon it might work for some cases
const std::shared_ptr<Sensor>
LmSensors::getPrimDefault()
{
    std::shared_ptr<Sensor> sensor = getSensor("it8728-isa-0a30 temp2");
    return sensor;    // core Package
}

// This is tailored for my needs other may need to look into config
//    but as pentiums are not that uncommon it might work for some cases
const std::shared_ptr<Sensor>
LmSensors::getSecDefault()
{
    return getSensor("it8728-isa-0a30 in5");        // core supply?
}

bool
LmSensors::isInited() const
{
    return m_init == DONE;
}