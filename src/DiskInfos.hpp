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

// represents the filesys & device infos

#pragma once

#include <glib-2.0/glib.h>
#include <string>
#include <map>

#include "Infos.hpp"
#include "libgtop_helper.h"
#include "DiskInfo.hpp"

class DiskInfos : public Infos {
public:
    DiskInfos();
    virtual ~DiskInfos();

    void update(int refreshRate, glibtop * glibtop) override;

    DiskInfo *getPrefered(std::string const &device) const;
    // get from device name of a partition the disk e.g. sda1 -> sda
    DiskInfo *getDisk(std::string const &device) const;

    const std::map<std::string, DiskInfo*> & getFilesyses();
    const std::map<std::string, DiskInfo*> & getDevices();

private:
    void updateDiskStat(int refreshRate, glibtop * glibtop);
    void updateMounts(int refresh_rate, glibtop * glibtop);

    void remove(DiskInfo *diskInfo);

    gint64 m_previous_diskstat_time;

    std::map<std::string, DiskInfo*> m_devices;
    std::map<std::string, DiskInfo*> m_mounts;

};

