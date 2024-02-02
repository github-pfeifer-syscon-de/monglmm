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

#include <memory>
#include <Geometry.hpp>
#include <Matrix.hpp>
#include <GraphShaderContext.hpp>
#include <Font.hpp>
#include <Text.hpp>
#include <Page.hpp>

class DiskInfos;    // forward declaration
class DiskInfo;    // forward declaration

class Filesyses : public Page {
public:
    Filesyses();
    explicit Filesyses(const Filesyses& fs) = delete;
    virtual ~Filesyses() = default;

    void update(GraphShaderContext *pGraph_shaderContext, TextContext* textCtx, Font *pFont, Matrix &persView, gint updateInterval);

    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;
    void setDiskInfos(const std::shared_ptr<DiskInfos>& diskInfos);

    Geometry *createDiskGeometry(const std::shared_ptr<DiskInfo>& diskInfo, GraphShaderContext *_ctx, TextContext *_txtCtx, Font *font, Matrix &persView, gint updateInterval);

private:
    float getOffset(const std::shared_ptr<DiskInfo>& partInfo, guint64 diffTime, gint updateInterval);

    std::shared_ptr<DiskInfos> m_diskInfos;

    static constexpr float FS_RADIUS = 0.5f;
};

