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

#pragma once

#include "DiskInfo.hpp"

#include <Font2.hpp>
#include <Geom2.hpp>
#include <Matrix.hpp>
#include <Text2.hpp>
#include <memory>

#include "Page.hpp"
#include "GraphShaderContext.hpp"

class DiskInfos;    // forward declaration
class DiskGeom;    // forward declaration

class Filesyses : public Page {
public:
    Filesyses();
    explicit Filesyses(const Filesyses& fs) = delete;
    virtual ~Filesyses() = default;

    void update(
          GraphShaderContext *pGraph_shaderContext
        , TextContext* textCtx
        , const psc::gl::ptrFont2& pFont
        , Matrix &persView
        , gint updateInterval);

    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;
    void setDiskInfos(const std::shared_ptr<DiskInfos>& diskInfos);
protected:
    void updateDiskGeometry(
          const psc::gl::aptrGeom2& geo
        , const std::shared_ptr<DiskGeom>& diskGeom
        , const std::shared_ptr<DiskInfo>& diskInfo
        , const std::vector<PtrMountInfo>& mntInfo
        , GraphShaderContext *_ctx
        , TextContext *_txtCtx
        , const psc::gl::ptrFont2& font
        , Matrix &persView
        , gint updateInterval);

private:
    float getColorOffsetRW(
          const std::shared_ptr<DiskInfo>& partInfo
        , guint64 diffTime
        , gint updateInterval);

    std::shared_ptr<DiskInfos> m_diskInfos;
    static constexpr auto FS_RADIUS{ 0.5f };
};

