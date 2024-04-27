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
    gint updateInterval) {
    Position pd(-3.3f, 3.0f, 0.0f);
    auto& fsMap = m_diskInfos->getFilesyses();
    float scale = std::min(1.0f, 3.0f / (float)fsMap.size());  // no upscaling
    for (auto& p : fsMap) {
        auto& diskInfo = p.second;
        auto geo = diskInfo->getGeometry();
        bool update = false;
        if (!geo) {
            geo = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, pGraph_shaderContext);
            pGraph_shaderContext->addGeometry(geo);
            diskInfo->setGeometry(geo);
            update = true;
        }
        if (update|| diskInfo->isChanged(updateInterval)) {
            updateDiskGeometry(geo, diskInfo, pGraph_shaderContext, textCtx, pFont, persView, updateInterval);
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

void
Filesyses::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    char tmp[64];
    double y = 10.0;
    for (auto p : m_diskInfos->getFilesyses()) {
        auto filesys = p.second;
        cr->move_to(1.0, y);
        float percent = filesys->getFreePercent();
        std::string size = Monitor::formatScale(filesys->getCapacityBytes(), "");
        snprintf(tmp, sizeof(tmp), "%10s %7s %4.1f%%", filesys->getMount().c_str(), size.c_str(), percent);
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
Filesyses::getOffset(const std::shared_ptr<DiskInfo>& partInfo, guint64 diffTime, gint updateInterval)
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
        r = (float)(dr / 2.0);                          // as we start from 0.5 with color make max hit 0.5
    }
    return r;
}

void
Filesyses::updateDiskGeometry(
          const psc::gl::aptrGeom2& geo
        , const std::shared_ptr<DiskInfo>& partInfo
        , GraphShaderContext *_ctx
        , TextContext *_txtCtx
        , const psc::gl::ptrFont2& font
        , Matrix &persView
        , gint updateInterval) {
    double usage = partInfo->getUsage();
    partInfo->setLastUsage(usage);
    partInfo->setLastReadTime(partInfo->getActualReadTime());
    partInfo->setLastWriteTime(partInfo->getActualWriteTime());
    float redOffs = getOffset(partInfo, partInfo->getActualWriteTime(), updateInterval);
    float greenOffs = getOffset(partInfo, partInfo->getActualReadTime(), updateInterval);
    //std::cout << this->dev << " Read " << (m_diffReadTime) << " greenOffs " << greenOffs << std::endl;
    //std::cout << this->dev << " Write " << (m_diffWriteTime) << " redOffs " << redOffs << std::endl;

    auto lgeo = geo.lease();
    if (lgeo) {
        lgeo->deleteVertexArray();
        auto geoName = Glib::ustring::sprintf("disk %s", partInfo->getMount());
        lgeo->setName(geoName);
        Color cu(0.5f + redOffs, 0.5f + greenOffs, 0.5f);   // idle gray, read green, write red
        lgeo->addCylinder(FS_RADIUS, 0.0f, usage, &cu);
        Color cf(0.2f, 0.9f, 0.2f);
        lgeo->addCylinder(FS_RADIUS, usage, 1.0, &cf);
        lgeo->create_vao();

        if (!partInfo->getDevText()) {
            auto devTxt = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, _ctx, font);
            auto ldevTxt = devTxt.lease();
            if (ldevTxt) {
                ldevTxt->setName(geoName + " dev");
                ldevTxt->setTextContext(_txtCtx);
                ldevTxt->setScale(0.0060f);
                Position p(-0.3f , +1.2f, 0.3f);
                ldevTxt->setPosition(p);
                const Glib::ustring dev(partInfo->getDevice());
                ldevTxt->setText(dev);
            }
            partInfo->setDevText(devTxt);
        }
        lgeo->addGeometry(partInfo->getDevText());
        if (!partInfo->getMountText()) {
            auto mountTxt = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, _ctx, font);
            auto lmountTxt = mountTxt.lease();
            if (lmountTxt) {
                lmountTxt->setName(geoName + " mnt");
                lmountTxt->setTextContext(_txtCtx);
                lmountTxt->setScale(0.0060f);
                Position p(-0.3f, +1.0f, 0.3f);
                lmountTxt->setPosition(p);
            }
            partInfo->setMountText(mountTxt);
        }
        double size = partInfo->getAvailBytes();
        auto ssize = Monitor::formatScale(size, "B");
        //std::cout << this->dev
        //          << " avail " << fsd.f_bavail
        //          << " frsize " << fsd.f_frsize
        //          << " size " << size
        //          << " ssize " << ssize
        //        << std::endl;
        Glib::ustring winfo = Glib::ustring::sprintf("%s %s", partInfo->getMount(),  ssize);
        auto mountTxt = partInfo->getMountText();
        auto lmountTxt = mountTxt.lease();
        if (lmountTxt) {
            lmountTxt->setText(winfo);
        }
        lgeo->addGeometry(mountTxt);
    }
}
