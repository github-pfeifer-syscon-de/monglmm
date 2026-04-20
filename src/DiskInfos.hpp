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

#include <glibmm.h>
#include <string>
#include <map>
#include <memory>
#include <Geom2.hpp>
#include <Text2.hpp>

#include "Infos.hpp"
#include "libgtop_helper.h"
#include "DiskInfo.hpp"

class DiskGeom
{
public:
    DiskGeom();
    explicit DiskGeom(const DiskGeom& other) = delete;
    virtual ~DiskGeom();

    void removeGeometry();

    void setGeometry(const psc::gl::aptrGeom2& _geometry);
    psc::gl::aptrGeom2 getGeometry() const;
    void setDevText(const psc::gl::aptrText2& _devTxt) {
        m_devTxt = _devTxt;
    }
    auto getDevText() const {
        return m_devTxt;
    }
    void setMountText(const psc::gl::aptrText2&  _mountTxt) {
        m_mountTxt = _mountTxt;
    }
    auto getMountText() const {
        return m_mountTxt;
    }

private:
    psc::gl::aptrGeom2 m_geometry;
    psc::gl::aptrText2 m_devTxt;
    psc::gl::aptrText2 m_mountTxt;

};

using PtrDiskGeom = std::shared_ptr<DiskGeom>;

class DiskInfos : public Infos
{
public:
    DiskInfos();
    virtual ~DiskInfos();

    void update(int refreshRate, glibtop * glibtop) override;
    bool updateDiskStat(gint64 diff);
    PtrDiskInfo getPrefered(std::string const &device) const;
    // get from device name of a partition the disk e.g. sda1 -> sda
    PtrDiskInfo getDisk(std::string const &device) const;

    const std::map<std::string, PtrMountInfo> & getFilesyses();
    std::vector<PtrMountInfo> getFilesyses(const std::string& device);
    const std::map<std::string, PtrDiskGeom>& getGeometries();
    const std::map<std::string, PtrDiskInfo> & getDevices();
    void removeDiskInfos();
    PtrDiskGeom createGeometry(const std::string& dev);
private:
    void updateDiskStat(int refreshRate, glibtop * glibtop);
    void updateMounts(int refresh_rate, glibtop * glibtop);

    gint64 m_previous_diskstat_time;

    std::map<std::string, PtrDiskInfo> m_devices;   // key is device e.g. sda1
    std::map<std::string, PtrDiskGeom> m_geom;      // key is device e.g. sda1
    std::map<std::string, PtrMountInfo> m_mounts;   // key is mount point e.g. /home

};

