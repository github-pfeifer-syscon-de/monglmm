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
/*
 1 - major number
 2 - minor number
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

class MountInfo;

using PtrMountInfo = std::shared_ptr<MountInfo>;

// this gives infos of mounted filesystems
//    -> using dev nodes
class MountInfo
{
public:
    MountInfo(const std::string& line);
    virtual ~MountInfo() = default;

    std::string getMount() const;
    //  e.g. sda
    std::string getDevName() const;
    bool isDev() const;
    static std::vector<PtrMountInfo> getMounts();
    bool refreshFilesys(glibtop * glibtop);
    // used ratio 0 = empty, 1 = full
    double getUsage();
    guint64 getCapacityBytes();
    double getAvailBytes();
    float getFreePercent();
    void reinit();
    static constexpr auto DEV_PREFIX{"/dev/"};
    bool isFilesysTouched() const {
        return m_filesysTouched;
    }
    void setFilesysTouched(bool _filesysTouched) {
        m_filesysTouched = _filesysTouched;
    }
    void setLastUsage(double _lastUsage) {
        m_lastUsage = _lastUsage;
    }
    double getLastUsage() const {
        return m_lastUsage;
    }
    bool isChanged();

    static constexpr auto USAGE_THRESHOLD{0.05};// some threshold 5% change in usage
protected:

private:
    bool m_filesysTouched;
    std::string m_dev;
    std::string m_mount;
    std::string m_type;
    std::string m_opt;
    int m_n1;
    int m_n2;
    struct statvfs fsd;
    double m_lastUsage{};
};

class DiskInfo;
using PtrDiskInfo = std::shared_ptr<DiskInfo>;

// these infos relate to dev nodes e.g. sda...sda4
class DiskInfo {
public:
    DiskInfo();
    virtual ~DiskInfo() = default;

    bool readStat(char const *line, gint64 delta_us);

    std::string getDevice() const {
        return m_device;
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
    bool isChanged(gint updateInterval);
    guint64 getTotalRWSectors();
    void reinit();
    static void getDiskStats(std::map<std::string, PtrDiskInfo>& map, int64_t diff_us);
private:
    struct disk_stat previous_disk_stat;
    std::string m_device;
    guint64 m_bytesReadPerS;
    guint64 m_bytesWrittenPerS;
    unsigned long m_actualReadTime;
    unsigned long m_actualWriteTime;
    unsigned long m_lastReadTime;
    unsigned long m_lastWriteTime;
};
