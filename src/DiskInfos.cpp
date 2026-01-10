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

#include <vector>
#include <fstream>
#include <iostream>

#include "libgtop_helper.h"
#ifdef LIBGTOP
#include <glibtop/mountlist.h>
#endif
#include "DiskInfos.hpp"
#include "FileByLine.hpp"
#include "Buffer.hpp"


class CompareByData  {
    public:
    bool operator()(const ptrDiskInfo &a, const ptrDiskInfo &b) {
        if (a == nullptr) {
            return FALSE;
        }
        if (b == nullptr) {
            return TRUE;
        }
        return a->getTotalRWSectors() > b->getTotalRWSectors();
    }
};



DiskInfos::DiskInfos()
: Infos()
, m_previous_diskstat_time{0l}
, m_devices()
, m_mounts()
{
}


DiskInfos::~DiskInfos()
{
    m_mounts.clear();
    m_devices.clear();
}

void DiskInfos::update(int refreshRate, glibtop * glibtop)
{
    updateDiskStat(refreshRate, glibtop);       // do this first as it shoud find all avail devices
    updateMounts(refreshRate, glibtop);         // this shoud assoicate the mounts
}

void
DiskInfos::updateDiskStat(int refreshRate, glibtop * glibtop)
{
    gint64 actual_time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
    gint64 diff = 0l;
    if (m_previous_diskstat_time > 0l) {
        diff = actual_time - m_previous_diskstat_time;
    }
    FileByLine fileByLine;
    if (!fileByLine.open("/proc/diskstats", "r")) {
        g_warning("DiskInfos: Could not open /proc/diskstats: %d, %s",
                  errno, strerror(errno));
        return;
    }
    // mark all as untouched
    for (auto devDisk = m_devices.begin(); devDisk != m_devices.end(); ++devDisk) {
        (devDisk->second)->setTouched(false);
    }
    size_t len = 0;
    while (TRUE) {
        const char *line = fileByLine.nextLine(&len);
        if (line == nullptr) {
            break;
        }
        if (len > 6) {
            DiskInfo tInfo;
            if (tInfo.readStat(line, 0l)) {
                auto devDisk = m_devices.find(tInfo.getDevice());
                ptrDiskInfo diskInfo;
                if (devDisk != m_devices.end()) {
                    diskInfo = devDisk->second;
                }
                else {
                    diskInfo = std::make_shared<DiskInfo>();
                    m_devices.insert(std::pair(tInfo.getDevice(), diskInfo));
                }
                diskInfo->readStat(line, diff);
            }
        }
    }
    m_previous_diskstat_time = actual_time;
    // remove those we did not touch
    for (auto devDisk = m_devices.begin(); devDisk != m_devices.end(); ) {
        auto diskInfo = devDisk->second;
        if (!diskInfo->isTouched()) {
            devDisk = m_devices.erase(devDisk);
            remove(diskInfo);
        }
        else {
            ++devDisk;
        }
    }
}

// get from device name of a partition the disk e.g. sda1 -> sda
//   expected map to be sorted e.g. sda before sda1 so we wont find sda1 from sda10
ptrDiskInfo
DiskInfos::getDisk(std::string const &device) const
{
    for (auto p : m_devices) {
        auto diskInfo = p.second;
        if (device.rfind(diskInfo->getDevice(), 0) == 0) {
            return diskInfo;
        }
    }
    return nullptr;
}

void
DiskInfos::updateMounts(int refresh_rate, glibtop * glibtop)
{
    for (auto p : m_devices) {
        auto diskInfo = p.second;
        diskInfo->setFilesysTouched(false);
    }
#ifdef LIBGTOP
    // needs to get adjusted
    glibtop_mountlist mount_list;
    glibtop_mountentry *mount_entry;
    mount_entry = glibtop_get_mountlist_l(glibtop, &mount_list, 1);
    if (mount_entry == nullptr)
        return TRUE;

    for (guint64 index = 0; index < mount_list.number; index++) {
        if (strncmp(mount_entry[index].devname, "/dev/", 5) == 0) {  // use only dev entries e.g. "real" filesystems
            mount_entry[index].mountdir
            std::string device(&mount_entry[index].devname[5]);
            auto p = m_devices.find(device);
            if (p != m_devices.end()) {
                DiskInfo *diskinfo = p->second;
                diskinfo->refreshFilesys(mount_entry[index].mountdir, glibtop);
            }
            else {
                std::cout << "Unexpected found fs " << mount_entry[index].mountdir << " but no dev " << mount_entry[index].devname << std::endl;
            }
        }
    }
    g_free(mount_entry);
#else

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
                    std::istringstream is(line);
                    std::string dev;
                    std::string mount;
                    std::string type;
                    std::string opt;
                    int n1,n2;
                    is >> dev
                       >> mount
                       >> type
                       >> opt
                       >> n1
                       >> n2;
                    if (dev.rfind("/dev/", 0) == 0) {   // this shoud give us "real" mounts
                        std::string devName = dev.substr(5);
                        auto p = m_devices.find(devName);
                        if (p != m_devices.end()) {
                            auto diskInfo = p->second;
                            auto m = m_mounts.find(mount);
                            if (m == m_mounts.end()) {      // add to mounts if needed
                                m_mounts.insert(std::pair(mount, diskInfo));
                                diskInfo->setMount(mount);
                            }
                            diskInfo->refreshFilesys(glibtop);
                        }
                        else {
                            std::cout << "Unexpected found fs " << mount << " but no dev " << dev << std::endl;
                        }
                    }
                }
            }
            mnts.close();
        }
    }
    catch (std::ios_base::failure& e) {
        g_warning("monitors: Could not read /proc/mounts: %d, %s",
                  errno, strerror(errno));
    }
#endif
    for (auto p : m_devices) {
        auto filesys = p.second;
        if (!filesys->isFilesysTouched()
          && filesys->getMount().length() > 0) {    // if device is unmounted keep device, but remove mount
            auto m = m_mounts.find(filesys->getMount());
            if (m != m_mounts.end()) {
                //std::cout << "Removing fs " << filesys->getMount() << " with dev " << filesys->getMount() << std::endl;
                m_mounts.erase(m);
                filesys->setMount("");
                filesys->removeGeometry();
            }
        }
    }
}


void
DiskInfos::remove(ptrDiskInfo diskInfo)
{
    // keep structures in sync
    if (diskInfo->getDevice() != "") {
        auto dev = m_devices.find(diskInfo->getDevice());
        if (dev != m_devices.end()) {
            m_devices.erase(dev);
        }
    }
    if (diskInfo->getMount() != "") {
        auto mnt = m_mounts.find(diskInfo->getMount());
        if (mnt != m_mounts.end()) {
            m_mounts.erase(mnt);
        }
    }
}

ptrDiskInfo
DiskInfos::getPrefered(std::string const &device) const
{
    ptrDiskInfo diskInfo;
    if (!device.empty()) {
        auto dev = m_devices.find(device);
        if (dev != m_devices.end()) {
            diskInfo = dev->second;
        }
    }
    if (!diskInfo) {
        std::vector<ptrDiskInfo> devs;
        for (auto dev : m_devices) {
            auto diskInf = dev.second;
            devs.push_back(diskInf);
        }
        std::sort(devs.begin(), devs.end(), CompareByData());
        auto i = devs.begin();
        if (i != devs.end()) {
            diskInfo = *i;   // select dev with most sectors r/w, will prefere disks before part
        }
    }
    return diskInfo;
}

const std::map<std::string, ptrDiskInfo> &
DiskInfos::getFilesyses()
{
    return m_mounts;
}


const std::map<std::string, ptrDiskInfo> &
DiskInfos::getDevices()
{
    return m_devices;
}

void
DiskInfos::removeDiskInfos()
{
    m_mounts.clear();
    m_devices.clear();
}
