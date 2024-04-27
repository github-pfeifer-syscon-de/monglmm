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

DiskInfo::DiskInfo()
: m_touched{true}
, m_bytesReadPerS{0l}
, m_bytesWrittenPerS{0l}
, m_lastUsage{-1.0}  //  ?
, m_actualReadTime{0l}
, m_actualWriteTime{0l}
, m_lastReadTime{0l}
, m_lastWriteTime{0l}
{
    reinit();
}

DiskInfo::~DiskInfo()
{
    removeGeometry();
}

void
DiskInfo::removeGeometry()
{
    m_devTxt.resetAll();
    m_mountTxt.resetAll();
    m_geometry.resetAll();
}

void
DiskInfo::reinit()
{
   // So we wont pickup sum on reactivation ???
   std::memset(&previous_disk_stat, 0, sizeof(struct disk_stat));
   std::memset(&fsd, 0, sizeof(struct statvfs));
}

bool
DiskInfo::readStat(char const *line, gint64 delta_us)
{
    struct disk_stat tdisk;
    char tdev[80];
    int fscanf_result = sscanf(line, " %lu %lu %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                &tdisk.major,&tdisk.minor,tdev,&tdisk.reads,&tdisk.rmerged,&tdisk.rsect,&tdisk.rtime,&tdisk.writes,&tdisk.wmerged,&tdisk.wsect,&tdisk.wtime,&tdisk.curr,&tdisk.iotime,&tdisk.weightime);
    if (fscanf_result >= 14)
    {
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
        }
        /* Copy current to previous. */
        memcpy(&previous_disk_stat, &tdisk, sizeof(struct disk_stat));
        m_actualReadTime = disk_delta.rtime;
        m_actualWriteTime = disk_delta.wtime;

        setTouched(true);
        return true;
    }
    return false;
}

bool
DiskInfo::refreshFilesys(glibtop * glibtop)
{
    // does not consume much time
    if (statvfs(m_mount.c_str(), &fsd) < 0) {
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


void
DiskInfo::setGeometry(const psc::gl::aptrGeom2& _geometry)
{
    m_geometry = _geometry;
}

psc::gl::aptrGeom2
DiskInfo::getGeometry() const
{
    return m_geometry;
}

double
DiskInfo::getUsage() {
    //std::cout << dev
    //         << " " << mount
    //         << " size: " << fsd.f_blocks * fsd.f_frsize / (1024l*1024l) << "M"
    //         << " avail: " << fsd.f_bavail * fsd.f_frsize / (1024l*1024l) << "M"
    //         << std::endl;

    return (double)(fsd.f_blocks - fsd.f_bavail) / (double)fsd.f_blocks;
}

guint64
DiskInfo::getCapacityBytes()
{
    return (guint64)fsd.f_blocks * (guint64)fsd.f_bsize;
}

double
DiskInfo::getAvailBytes()
{
    return (double)((guint64)fsd.f_bavail * (guint64)fsd.f_frsize);    // long cast is for compat 32bit
}

float
DiskInfo::getFreePercent()
{
    if (fsd.f_ffree <= 0) {
        return 0.0;
    }
    return (float)((double)fsd.f_bavail / (double)fsd.f_blocks * 100.0);
}

bool
DiskInfo::isChanged(gint updateInterval)
{
    double diff = std::abs(getUsage() - m_lastUsage);
    long diffRead = std::abs((gint64)m_lastReadTime - (gint64)getActualReadTime());
    long diffWrite = std::abs((gint64)m_lastWriteTime - (gint64)getActualWriteTime());
    long ioThreshold = updateInterval * 10;  // interval is seconds  (io is ms ) -> *1000, want percent -> /100  = *10
    if (diff > 0.01                 // some threshold 1% change in usage / 1% of io time
        || diffRead > ioThreshold
        || diffWrite > ioThreshold) {
        return true;
    }
    return false;
}


guint64
DiskInfo::getTotalRWSectors()
{
    return previous_disk_stat.rsect + previous_disk_stat.wsect;
}