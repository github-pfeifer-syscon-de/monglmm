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

#include <glib-2.0/glib.h>
#include <string>
#include <sys/statvfs.h>

#include "libgtop_helper.h"
#include <Geom2.hpp>
#include <Text2.hpp>

/*
 1 - major number
 2 - minor mumber
 3 - device name
 4 - reads completed successfully
 5 - reads merged
 6 - sectors read
 7 - time spent reading (ms)
 8 - writes completed
 9 - writes merged
10 - sectors written
11 - time spent writing (ms)
12 - I/Os currently in progress
13 - time spent doing I/Os (ms)
14 - weighted time spent doing I/Os (ms)
*/
struct disk_stat {
    unsigned long major,minor,reads,rmerged,rsect,rtime,writes,wmerged,wsect,wtime,curr,iotime,weightime;
};


class DiskInfo {
public:
    DiskInfo();
    virtual ~DiskInfo();

    void removeGeometry();
    void reinit();

    bool readStat(char const *line, gint64 delta_us);
    bool refreshFilesys(glibtop * glibtop);

    const std::string &getDevice() const {
        return m_device;
    }
    const std::string &getMount() const {
        return m_mount;
    }
    void setMount(const std::string &mount) {
        m_mount = mount;
    }
    bool isTouched() const {
        return m_touched;
    }
    void setTouched(bool _touched) {
        m_touched = _touched;
    }
    bool isFilesysTouched() const {
        return m_filesysTouched;
    }
    void setFilesysTouched(bool _filesysTouched) {
        m_filesysTouched = _filesysTouched;
    }
    guint64 getBytesReadPerS() const {
        return m_bytesReadPerS;
    }
    guint64 getBytesWrittenPerS() const  {
        return m_bytesWrittenPerS;
    }
    unsigned long getActualReadTime() const {
        return m_actualReadTime;
    }
    unsigned long getActualWriteTime() const {
        return m_actualWriteTime;
    }
    void setLastReadTime(unsigned long lastReadTime) {
        m_lastReadTime = lastReadTime;
    }
    void setLastWriteTime(unsigned long lastWriteTime) {
        m_lastWriteTime = lastWriteTime;
    }
    double getUsage();

    void setGeometry(const psc::gl::aptrGeom2& _geometry);
    auto getGeometry() const {
        return m_geometry;
    }
    void setDevText(const psc::gl::aptrText2& _devTxt) {
        m_devTxt = _devTxt;
    }
    auto getDevText() const {
        return m_devTxt;
    }
    void setMountText(const psc::gl::aptrText2&  _MountTxt) {
        m_mountTxt = _MountTxt;
    }
    auto getMountText() const {
        return m_mountTxt;
    }
    void setLastUsage(double _lastUsage) {
        m_lastUsage = _lastUsage;
    }
    double getLastUsage() const {
        return m_lastUsage;
    }

    guint64 getCapacityBytes();
    double getAvailBytes();
    bool isChanged(gint updateInterval);
    float getFreePercent();
    guint64 getTotalRWSectors();
private:
    struct disk_stat previous_disk_stat;

    std::string m_device;
    std::string m_mount;
    bool m_touched;
    bool m_filesysTouched;
    guint64 m_bytesReadPerS;
    guint64 m_bytesWrittenPerS;
    struct statvfs fsd;
    double m_lastUsage;
    psc::gl::aptrGeom2 m_geometry;
    psc::gl::aptrText2 m_devTxt;
    psc::gl::aptrText2 m_mountTxt;
    unsigned long m_actualReadTime;
    unsigned long m_actualWriteTime;
    unsigned long m_lastReadTime;
    unsigned long m_lastWriteTime;

};

