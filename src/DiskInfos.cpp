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
    bool operator()(const PtrDiskInfo &a, const PtrDiskInfo &b) {
        if (a == nullptr) {
            return FALSE;
        }
        if (b == nullptr) {
            return TRUE;
        }
        return a->getTotalRWSectors() > b->getTotalRWSectors();
    }
};

DiskGeom::DiskGeom()
{
}

DiskGeom::~DiskGeom()
{
    removeGeometry();
}

void
DiskGeom::removeGeometry()
{
    m_devTxt.resetAll();
    m_mountTxt.resetAll();
    m_geometry.resetAll();
}
void
DiskGeom::setGeometry(const psc::gl::aptrGeom2& _geometry)
{
    m_geometry = _geometry;
}

psc::gl::aptrGeom2
DiskGeom::getGeometry() const
{
    return m_geometry;
}


DiskInfos::DiskInfos()
: Infos()
, m_previous_diskstat_time{}
{
}


DiskInfos::~DiskInfos()
{
    m_mounts.clear();
    m_geom.clear();
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
    gint64 diff_us{};
    if (m_previous_diskstat_time > 0l) {
        diff_us = actual_time - m_previous_diskstat_time;
    }
    DiskInfo::getDiskStats(m_devices, diff_us);
    m_previous_diskstat_time = actual_time;
    // remove geom we don't have a device for
    for (auto iterGeom = m_geom.begin(); iterGeom != m_geom.end(); ) {
        auto diskGeom = iterGeom->second;
        if (m_devices.find(iterGeom->first) == m_devices.end()) {
            //std::cout << "Remove device not touched " << iterGeom->first << std::endl;
            iterGeom = m_geom.erase(iterGeom);
        }
        else {
            ++iterGeom;
        }
    }
}

// get from device name of a partition the disk e.g. sda1 -> sda
//   expected map to be sorted e.g. sda before sda1 so we won't find sda1 from sda10
PtrDiskInfo
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

PtrDiskGeom
DiskInfos::createGeometry(const std::string& dev)
{
    auto ptrGeom = std::make_shared<DiskGeom>();
    m_geom.insert(std::pair(dev, ptrGeom));
    return ptrGeom;
}

void
DiskInfos::updateMounts(int refresh_rate, glibtop * glibtop)
{
    for (auto p : m_mounts) {
        auto& diskInfo = p.second;
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
    for (auto& mnt : MountInfo::getMounts()) {
        auto mount = mnt->getMount();
        auto devName = mnt->getDevName();
        auto p = m_devices.find(devName);
        if (p != m_devices.end()) { // only use if we have a device as these are out display unit
            auto m = m_mounts.find(mount);
            PtrMountInfo ptrMount;
            if (m == m_mounts.end()) {      // add to mounts if needed
                m_mounts.insert(std::pair(mount, mnt));
                ptrMount = mnt;
            }
            else {
                ptrMount = m->second;
            }
            ptrMount->refreshFilesys(glibtop);
        }
        else {
            std::cout << "Unexpected found fs " << mount << " but no dev " << devName << std::endl;
        }
    }
#endif
    std::set<std::string> touchedDevices;
    for (auto imap = m_mounts.begin(); imap != m_mounts.end(); ) {
        auto& filesys = imap->second;
        if (!filesys->isFilesysTouched()) {    // if device is unmounted keep device, but remove from display
            imap = m_mounts.erase(imap);
        }
        else {
            //std::cout << "Collecting dev " << filesys->getDevName() << std::endl;
            touchedDevices.insert(filesys->getDevName());
            ++imap;
        }
    }
    for (auto dev : m_geom) {
        if (!touchedDevices.contains(dev.first)) {  // since we have no association to filesys don't show device
            auto& diskGeo = dev.second;
            //std::cout << "DiskInfos::updateMounts removing devGeo first " << dev.first << " dev " << dev.first << std::endl;
            diskGeo->removeGeometry();
        }
    }
}

PtrDiskInfo
DiskInfos::getPrefered(std::string const &device) const
{
    PtrDiskInfo diskInfo;
    if (!device.empty()) {
        auto dev = m_devices.find(device);
        if (dev != m_devices.end()) {
            diskInfo = dev->second;
        }
    }
    if (!diskInfo) {
        std::vector<PtrDiskInfo> devs;
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

const std::map<std::string, PtrMountInfo> &
DiskInfos::getFilesyses()
{
    return m_mounts;
}

std::vector<PtrMountInfo>
DiskInfos::getFilesyses(const std::string& device)
{
    std::vector<PtrMountInfo> mnts;
    mnts.reserve(4);
    for (auto fs : m_mounts) {
        if (fs.second->getDevName() == device) {
            mnts.push_back(fs.second);
        }
    }
    return mnts;
}

const std::map<std::string, PtrDiskGeom>&
DiskInfos::getGeometries()
{
    return m_geom;
}

const std::map<std::string, PtrDiskInfo> &
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
