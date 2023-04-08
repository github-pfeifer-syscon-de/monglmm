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
#include <math.h>

#include "TextContext.hpp"
#include "Filesyses.hpp"
#include "DiskInfos.hpp"
#include "Monitor.hpp"
#include "StringUtils.hpp"

Filesyses::Filesyses()
: m_diskInfos{nullptr}
{
}

Filesyses::~Filesyses()
{
}


void
Filesyses::update(GraphShaderContext *pGraph_shaderContext, TextContext* textCtx, Font *pFont, Matrix &persView, gint updateInterval) {
    Position pd(-3.3f, 3.0f, 0.0f);
    auto fsMap = m_diskInfos->getFilesyses();
    float scale = std::min(1.0f, 3.0f / (float)fsMap.size());  // no upscaling
    for (auto p : fsMap) {
        DiskInfo *diskInfo = p.second;
        Geometry *geo = diskInfo->getGeometry();
        if (diskInfo->isChanged(updateInterval)) {
            geo = createDiskGeometry(diskInfo, pGraph_shaderContext, textCtx, pFont, persView, updateInterval);
            pGraph_shaderContext->addGeometry(geo);
        }
        geo->setPosition(pd);   // always update position, as we may have changes (e.g. unmount)
        geo->setScale(scale);
        pd.y -= scale * 2.0f;
    }
}

void
Filesyses::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    char tmp[64];
    double y = 10.0;
    for (auto p : m_diskInfos->getFilesyses()) {
        DiskInfo *filesys = p.second;
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
Filesyses::setDiskInfos(DiskInfos *diskInfos)
{
    m_diskInfos = diskInfos;
}



float
Filesyses::getOffset(DiskInfo *partInfo, guint64 diffTime, gint updateInterval)
{
    float r = 0.0f;
    DiskInfo *diskInfo = m_diskInfos->getDisk(partInfo->getDevice()) ;
    if (diskInfo == nullptr) {
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

Geometry *
Filesyses::createDiskGeometry(DiskInfo *partInfo, GraphShaderContext *_ctx, TextContext *_txtCtx, Font *font, Matrix &persView, gint updateInterval) {
    double usage = partInfo->getUsage();
    partInfo->setLastUsage(usage);
    partInfo->setLastReadTime(partInfo->getActualReadTime());
    partInfo->setLastWriteTime(partInfo->getActualWriteTime());
    float redOffs = getOffset(partInfo, partInfo->getActualWriteTime(), updateInterval);
    float greenOffs = getOffset(partInfo, partInfo->getActualReadTime(), updateInterval);
    //std::cout << this->dev << " Read " << (m_diffReadTime) << " greenOffs " << greenOffs << std::endl;
    //std::cout << this->dev << " Write " << (m_diffWriteTime) << " redOffs " << redOffs << std::endl;

    Geometry *geo = new Geometry(GL_TRIANGLES, _ctx);
    geo->setRemoveChildren(false);

    Color cu(0.5f + redOffs, 0.5f + greenOffs, 0.5f);   // idle gray, read green, write red
    geo->addCylinder(FS_RADIUS, 0.0f, usage, &cu);
    Color cf(0.2f, 0.9f, 0.2f);
    geo->addCylinder(FS_RADIUS, usage, 1.0, &cf);
    geo->create_vao();

    if (partInfo->getDevText() == nullptr) {
        Text *devTxt = _txtCtx->createText(_ctx, font);
        devTxt->setScale(0.0060f);
        Position p(-0.3f , +1.2f, 0.3f);
        devTxt->setPosition(p);
        const Glib::ustring dev(partInfo->getDevice());
        devTxt->setText(dev);
        partInfo->setDevText(devTxt);
    }
    geo->addGeometry(partInfo->getDevText());
    if (partInfo->getMountText() == nullptr) {
        Text *mountTxt = _txtCtx->createText(_ctx, font);
        mountTxt->setScale(0.0060f);
        Position p(-0.3f, +1.0f, 0.3f);
        mountTxt->setPosition(p);
        partInfo->setMountText(mountTxt);
    }
    double size = partInfo->getAvailBytes();
    std::string ssize = Monitor::formatScale(size, "B");
    //std::cout << this->dev
    //          << " avail " << fsd.f_bavail
    //          << " frsize " << fsd.f_frsize
    //          << " size " << size
    //          << " ssize " << ssize
    //        << std::endl;
    std::ostringstream oss1;
    oss1 << partInfo->getMount() << " " << ssize;
    std::string info = oss1.str();
    Glib::ustring winfo(info);
    partInfo->getMountText()->setText(winfo);
    geo->addGeometry(partInfo->getMountText());

    partInfo->setGeometry(geo);
    return geo;
}
