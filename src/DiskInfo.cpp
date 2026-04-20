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

#include <gtkmm.h>
#include <glibmm.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>

#include "libgtop_helper.h"
#ifdef LIBGTOP
#include <glibtop/fsusage.h>
#endif

#include "Monitor.hpp"
#include "DiskInfo.hpp"
#include "FileByLine.hpp"

MountInfo::MountInfo(const std::string& line)
{
    std::istringstream is(line);
    is >> m_dev
       >> m_mount
       >> m_type
       >> m_opt
       >> m_n1
       >> m_n2;
}

std::string
MountInfo::getMount() const
{
    return m_mount;
}

std::string
MountInfo::getDevName() const
{
    return m_dev.substr(5);
}

bool
MountInfo::isDev() const
{
    return m_dev.rfind(DEV_PREFIX, 0) == 0;   // this should give us "real" devices/mounts
}

bool
MountInfo::refreshFilesys(glibtop * glibtop)
{
    // does not consume much time
    if (statvfs(m_mount.c_str(), &fsd) < 0) {
        std::cout << "Error querying fs " << m_mount << std::endl;
        return false;
    }
    setFilesysTouched(true);

// as statvfs is posix , leave this as an optional extension
//            glibtop_fsusage fsusage;
//            glibtop_get_fsusage (&fsusage, mount_entry[index].mountdir);
//            printf ("Mount_Entry: %-30s %-10s %-20s\n",
//                    mount_entry[index].mountdir,
//                    mount_entry[index].type,
//                    mount_entry[index].devname);
//            printf ("Blocks %9" G_GUINT64_FORMAT "\n"
//                    "Free   %9" G_GUINT64_FORMAT "\n"
//                    "Avail  %9" G_GUINT64_FORMAT "\n"
//                    "Files  %9" G_GUINT64_FORMAT "\n"
//                    "FiFree %9" G_GUINT64_FORMAT "\n"
//                    "Blocks %9d\n"
//                    "Read   %9" G_GUINT64_FORMAT "\n"
//                    "Write  %9" G_GUINT64_FORMAT "\n",
//                    fsusage.blocks, fsusage.bfree,
//                    fsusage.bavail, fsusage.files,
//                    fsusage.ffree, fsusage.block_size,
//                    fsusage.read, fsusage.write);
    return true;
}

double
MountInfo::getUsage()
{
    //std::cout << dev
    //         << " " << mount
    //         << " size: " << fsd.f_blocks * fsd.f_frsize / (1024l*1024l) << "M"
    //         << " avail: " << fsd.f_bavail * fsd.f_frsize / (1024l*1024l) << "M"
    //         << std::endl;

    return (double)(fsd.f_blocks - fsd.f_bavail) / (double)fsd.f_blocks;
}

guint64
MountInfo::getCapacityBytes()
{
    return (uint64_t)fsd.f_blocks * (uint64_t)fsd.f_bsize;
}

double
MountInfo::getAvailBytes()
{
    return (double)(static_cast<uint64_t>(fsd.f_bavail)
            * static_cast<uint64_t>(fsd.f_frsize));    // long cast is for compat 32bit
}

float
MountInfo::getFreePercent()
{
    if (fsd.f_blocks <= 0) {
        return 0.0;
    }
    return static_cast<float>((static_cast<double>(fsd.f_bavail)
            / static_cast<double>(fsd.f_blocks)) * 100.0);
}

void
MountInfo::reinit()
{
    std::memset(&fsd, 0, sizeof(struct statvfs));
}

bool
MountInfo::isChanged()
{
    auto usageDiff = std::abs(getUsage() - m_lastUsage);
    return usageDiff > USAGE_THRESHOLD;
}

std::vector<PtrMountInfo>
MountInfo::getMounts()
{
    std::vector<PtrMountInfo> ret;
    ret.reserve(16);
    std::ifstream mnts;
    std::ios_base::iostate exceptionMask = mnts.exceptions() | std::ios::failbit | std::ios::badbit;
    mnts.exceptions(exceptionMask);
    try {
        mnts.open("/proc/mounts");
        if (mnts.is_open()) {
            while (!mnts.eof()
                && mnts.peek() >= 0) {   // try to read ahead
                std::string line;
                std::getline(mnts, line);
                if (line.length() > 8) {
                    auto mountInfo = std::make_shared<MountInfo>(line);
                    if (mountInfo->isDev()) {
                        ret.emplace_back(std::move(mountInfo));
                    }
                }
            }
        }
    }
    catch (std::ios_base::failure& e) {
        g_warning("monitors: Could not read /proc/mounts: %d, %s",
                  errno, strerror(errno));
    }
    if (mnts.is_open()) {
        mnts.close();
    }
    return ret;
}


DiskInfo::DiskInfo()
: m_bytesReadPerS{}
, m_bytesWrittenPerS{}
, m_actualReadTime{}
, m_actualWriteTime{}
, m_lastReadTime{}
, m_lastWriteTime{}
{
    reinit();
}

void
DiskInfo::reinit()
{
    // So we won't pick up sum on reactivation ???
    m_bytesReadPerS = 0ul;
    m_bytesWrittenPerS = 0ul;
}

bool
DiskInfo::readStat(char const *line, gint64 delta_us)
{
    struct disk_stat tdisk;
    char tdev[80];
    int fscanf_result = sscanf(line, " %lu %lu %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                &tdisk.major,&tdisk.minor,tdev,&tdisk.reads,&tdisk.rmerged,&tdisk.rsect,&tdisk.rtime,&tdisk.writes,&tdisk.wmerged,&tdisk.wsect,&tdisk.wtime,&tdisk.curr,&tdisk.iotime,&tdisk.weightime);
    if (fscanf_result >= 14) {
        m_device = tdev;    // is  without "/dev/"
        struct disk_stat disk_delta;
        // what do about wrapping ? -> it will give us a huge peak :)
        disk_delta.reads = tdisk.reads - previous_disk_stat.reads;
        disk_delta.rmerged = tdisk.rmerged - previous_disk_stat.rmerged;
        disk_delta.rsect = tdisk.rsect - previous_disk_stat.rsect;
        disk_delta.rtime = tdisk.rtime - previous_disk_stat.rtime;
        disk_delta.writes = tdisk.writes - previous_disk_stat.writes;
        disk_delta.wmerged = tdisk.wmerged - previous_disk_stat.wmerged;
        disk_delta.wsect = tdisk.wsect - previous_disk_stat.wsect;
        disk_delta.wtime = tdisk.wtime - previous_disk_stat.wtime;
        disk_delta.iotime = tdisk.iotime - previous_disk_stat.iotime;
        disk_delta.weightime = tdisk.weightime - previous_disk_stat.weightime;
        unsigned long writeValue = disk_delta.wsect;
        unsigned long readValue = disk_delta.rsect;
        if (delta_us > 0l) {    // for the first call time will not be valid and values will jump from 0 to all
            // only update after init
            double delta_sect_s = 512.0E6/(double)delta_us;        // factor that combines conversion sectors per time unit to byte/s

            m_bytesReadPerS = (guint64)(readValue * delta_sect_s);
            m_bytesWrittenPerS = (guint64)(writeValue * delta_sect_s);
            //std::cout << "DiskInfo::readStat " << m_device
            //          << " diff " << delta_us << "us"
            //          <<  " read " << tdisk.rsect << "sect prev " <<  previous_disk_stat.rsect << "sect"
            //          << " " << readValue << "sect"
            //          << " " << m_bytesReadPerS << "B/s"
            //          << " write " << tdisk.wsect << "sect prev " <<  previous_disk_stat.wsect << "sect"
            //          << " " << writeValue << "sect"
            //          << " " << m_bytesWrittenPerS << "B/s" << std::endl;
        }
        /* Copy current to previous. */
        previous_disk_stat = tdisk;
        m_actualReadTime = disk_delta.rtime;
        m_actualWriteTime = disk_delta.wtime;
        return true;
    }
    return false;
}

bool
DiskInfo::isChanged(gint updateInterval)
{
    auto diffRead = std::abs((gint64)m_lastReadTime - (gint64)getActualReadTime());
    auto diffWrite = std::abs((gint64)m_lastWriteTime - (gint64)getActualWriteTime());
    long ioThreshold = updateInterval * 10;  // interval is seconds  (io is ms ) -> *1000, want percent -> /100  = *10
    if (diffRead > ioThreshold
     || diffWrite > ioThreshold) {  // 1% of io time
        return true;
    }
    return false;
}

guint64
DiskInfo::getTotalRWSectors()
{
    return previous_disk_stat.rsect + previous_disk_stat.wsect;
}

void
DiskInfo::getDiskStats(std::map<std::string, PtrDiskInfo>& map, int64_t diff_us)
{
    FileByLine fileByLine;
    if (!fileByLine.open("/proc/diskstats", "r")) {
        std::cout << "DiskInfos: Could not open /proc/diskstats: " << errno << " " << strerror(errno) << std::endl;
        return;
    }
    size_t len{};
    std::set<std::string> foundDev;
    while (true) {
        const char *line = fileByLine.nextLine(&len);
        if (line == nullptr) {
            break;
        }
        if (len > 6) {
            DiskInfo tInfo;
            if (tInfo.readStat(line, 0l)) {
                auto dev = map.find(tInfo.getDevice());
                PtrDiskInfo pInfo;
                if (dev == map.end()) {
                    pInfo = std::make_shared<DiskInfo>();
                    map.insert(std::pair(tInfo.getDevice(), pInfo));
                }
                else {
                    pInfo = dev->second;
                }
                pInfo->readStat(line, diff_us);
                foundDev.insert(tInfo.getDevice());
            }
        }
    }
    for (auto entry = map.begin(); entry != map.end(); ) {  // remove obsolet
        if (!foundDev.contains(entry->first)) {
            entry = map.erase(entry);
        }
        else {
            ++entry;
        }
    }
}