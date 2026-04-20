/*
 * Copyright (C) 2019 rpf
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


#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <gtkmm.h>
#include <cmath>
#include <source_location>
#include <TextContext.hpp>
#include <StringUtils.hpp>
#include <psc_format.hpp>

#include "Filesyses.hpp"
#include "DiskInfos.hpp"
#include "Monitor.hpp"


Filesyses::Filesyses()
{
}

void
Filesyses::update(
    GraphShaderContext *pGraph_shaderContext,
    TextContext* textCtx,
    const psc::gl::ptrFont2& pFont,
    Matrix &persView,
    gint updateInterval)
{
    Position pd(-3.3f, 3.0f, 0.0f);
    float cnt{};
    auto& mapDevices = m_diskInfos->getDevices();
    for (auto entryDev : mapDevices) {
        auto mntInfos = m_diskInfos->getFilesyses(entryDev.first);
        if (!mntInfos.empty()) {
            ++cnt;
        }
    }
    auto& geoMap = m_diskInfos->getGeometries();
    float scale = std::min(1.0f, 3.0f / cnt);  // no upscaling
    for (auto entryDev : mapDevices) {
        auto mntInfos = m_diskInfos->getFilesyses(entryDev.first);
        if (!mntInfos.empty()) {
            auto iterGeo  = geoMap.find(entryDev.first);
            PtrDiskGeom diskGeom;
            if (iterGeo != geoMap.end()) {
                diskGeom = iterGeo->second;
            }
            else {
                diskGeom = m_diskInfos->createGeometry(entryDev.first);
            }
            auto devInfo = entryDev.second;
            auto geo = diskGeom->getGeometry();
            bool update = false;
            if (!geo) {
                geo = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, pGraph_shaderContext);
                pGraph_shaderContext->addGeometry(geo);
                diskGeom->setGeometry(geo);
                update = true;
            }
            for (auto mntInfo : mntInfos) {
                update |= mntInfo->isChanged();
            }
            if (update||devInfo->isChanged(updateInterval)) {
                updateDiskGeometry(geo, diskGeom, devInfo, mntInfos, pGraph_shaderContext, textCtx, pFont, persView, updateInterval);
            }
            if (geo) {
                if (auto lgeo = geo.lease()) {
                    lgeo->setPosition(pd);   // always update position, as we may have changes (e.g. unmount)
                    lgeo->setScale(scale);
                }
            }
            pd.y -= scale * 2.0f;
        }
    }
}

void
Filesyses::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    double y = 10.0;
    for (auto p : m_diskInfos->getFilesyses()) {
        auto filesys = p.second;
        cr->move_to(1.0, y);
        float percent = filesys->getFreePercent();
        std::string size = Monitor::formatScale(filesys->getCapacityBytes(), "");
        auto tmp = psc::fmt::format("{:10} {:7} {:4.1f}%", filesys->getMount(), size, percent);
        cr->show_text(tmp);
        y += 10;
        if (y > height)
            break;
    }
}

void
Filesyses::setDiskInfos(const std::shared_ptr<DiskInfos>& diskInfos)
{
    m_diskInfos = diskInfos;
}



float
Filesyses::getColorOffsetRW(
      const std::shared_ptr<DiskInfo>& partInfo
    , guint64 diffTime
    , gint updateInterval)
{
    float r = 0.0f;
    auto diskInfo = m_diskInfos->getDisk(partInfo->getDevice()) ;
    if (!diskInfo) {
        //std::cerr << "Cound not find disk for " << partInfo->getDevice() << std::endl;
        diskInfo = partInfo;
    }
    guint64 max = std::max(diskInfo->getActualReadTime(), diskInfo->getActualWriteTime());
    if (max > 0) {
        double dr = (double)diffTime / (double)max;     // 1 for 100% of io time
        r = static_cast<float>(dr / 2.0);               // as we start from 0.5 with color make max hit 0.5
    }
    return r;
}

void
Filesyses::updateDiskGeometry(
          const psc::gl::aptrGeom2& geo
        , const std::shared_ptr<DiskGeom>& diskGeom
        , const std::shared_ptr<DiskInfo>& diskInfo
        , const std::vector<PtrMountInfo>& mntInfos
        , GraphShaderContext *_ctx
        , TextContext *_txtCtx
        , const psc::gl::ptrFont2& font
        , Matrix &persView
        , gint updateInterval)
{
    double maxUsage{};
    double minSize{1e15}; //6=M,9=G,12=T,15=P
    Glib::ustring mounts;
    mounts.reserve(32);
    for (auto& mntInfo : mntInfos) {
        auto usage = mntInfo->getUsage();
        mntInfo->setLastUsage(usage);
        maxUsage = std::max(maxUsage, usage);
        if (!mounts.empty()) {
            mounts += " ";
        }
        mounts += mntInfo->getMount();
        double availSize = mntInfo->getAvailBytes();
        minSize = std::min(minSize, availSize); // as mounts sharing a device e.g. btrfs share a free size, this should work
    }
    diskInfo->setLastReadTime(diskInfo->getActualReadTime());
    diskInfo->setLastWriteTime(diskInfo->getActualWriteTime());
    float redOffs = getColorOffsetRW(diskInfo, diskInfo->getActualWriteTime(), updateInterval);
    float greenOffs = getColorOffsetRW(diskInfo, diskInfo->getActualReadTime(), updateInterval);
    //std::cout << this->dev << " Read " << (m_diffReadTime) << " greenOffs " << greenOffs << std::endl;
    //std::cout << this->dev << " Write " << (m_diffWriteTime) << " redOffs " << redOffs << std::endl;

    auto lgeo = geo.lease();
    if (lgeo) {
        lgeo->deleteVertexArray();
        auto geoName = Glib::ustring::sprintf("disk %s", diskInfo->getDevice());
        lgeo->setName(geoName);
        Color cu(0.5f + redOffs, 0.5f + greenOffs, 0.5f);   // idle gray, read green, write red
        lgeo->addCylinder(FS_RADIUS, 0.0f, maxUsage, &cu);
        Color cf(0.1f, 0.5f, 0.1f);
        lgeo->addCylinder(FS_RADIUS, maxUsage, 1.0, &cf);
        lgeo->create_vao();

        if (!diskGeom->getDevText()) {
            auto devTxt = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, _ctx, font);
            auto ldevTxt = devTxt.lease();
            if (ldevTxt) {
                ldevTxt->setName(geoName + " dev");
                ldevTxt->setTextContext(_txtCtx);
                ldevTxt->setScale(0.0060f);
                Position p(-0.1f , +1.3f, 0.5f);
                ldevTxt->setPosition(p);
                const Glib::ustring dev(diskInfo->getDevice());
                ldevTxt->setText(dev);
            }
            diskGeom->setDevText(devTxt);
        }
        lgeo->addGeometry(diskGeom->getDevText());
        if (!diskGeom->getMountText()) {
            auto mountTxt = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, _ctx, font);
            auto lmountTxt = mountTxt.lease();
            if (lmountTxt) {
                lmountTxt->setName(geoName + " mnt");
                lmountTxt->setTextContext(_txtCtx);
                lmountTxt->setScale(0.0070f);
                Position p(-0.75f, +1.1f, 0.5f);
                lmountTxt->setPosition(p);
            }
            diskGeom->setMountText(mountTxt);
        }
        auto ssize = Monitor::formatScale(minSize, "B");
        //std::cout << this->dev
        //          << " avail " << fsd.f_bavail
        //          << " frsize " << fsd.f_frsize
        //          << " size " << size
        //          << " ssize " << ssize
        //        << std::endl;
        Glib::ustring winfo =  mounts + "  " + ssize;
        auto mountTxt = diskGeom->getMountText();
        auto lmountTxt = mountTxt.lease();
        if (lmountTxt) {
            lmountTxt->setText(winfo);
        }
        lgeo->addGeometry(mountTxt);
    }
}
