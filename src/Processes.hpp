/*
 * Copyright (C) 2018 rpf
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

/*
 * Registry for all processes
 */

#pragma once

#include <map>
#include <iostream>
#include <memory>
#include <array>

#include "GraphShaderContext.hpp"
#include "Process.hpp"
#include "Font2.hpp"
#include "DiagramMonitor.hpp"
#include "Text2.hpp"

static const long ROOT_PID = 1l;

enum class TreeType {
    ARC = 'a',  // see also Processes::fromString
    BLOCK = 'b',
    LINE = 'l'
};

class Processes {
public:
    Processes(guint _size);
    virtual ~Processes();

    void resetProc();
    void update(std::shared_ptr<Monitor> cpu, std::shared_ptr<Monitor> mem);
    static constexpr auto TOP_PROC = 3u;
    void findMax(std::array<pProcess, TOP_PROC>& topMem
               , std::array<pProcess, TOP_PROC>& topCpu);
    void display(
            GraphShaderContext *pGraph_shaderContext,
            TextContext *_txtCtx,
            const psc::gl::ptrFont2& pFont,
            std::shared_ptr<DiagramMonitor> cpu,
            std::shared_ptr<DiagramMonitor> mem,
            Matrix &persView);
    void setTreeType(const Glib::ustring &uProcessType);
    void buildTree();
    void restore();


    static TreeType fromString(const Glib::ustring &str) {
        if (str == "b") {
            return TreeType::BLOCK;
        }
        if (str == "l") {
            return TreeType::LINE;
        }
        return TreeType::ARC;       // use some default
    }
    void printInfo();
    void displayTops(NaviContext* context, const Matrix &projView);
protected:
    void updateCpu(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, const psc::gl::ptrFont2& pFont, std::shared_ptr<DiagramMonitor> cpu, Matrix &persView, Position &p);
    void updateMem(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, const psc::gl::ptrFont2& pFont, std::shared_ptr<DiagramMonitor> mem, Matrix &persView, Position &p);

private:
    std::map<long, pProcess> mProcesses;
    std::array<pProcess, TOP_PROC> m_topMem;    // proc highest mem usage
    std::array<pProcess, TOP_PROC> m_topCpu;  // proc highest cpu usage
    std::array<psc::gl::aptrGeom2, TOP_PROC> m_memGeo;
    std::array<psc::gl::aptrGeom2, TOP_PROC> m_cpuGeo;
    std::array<psc::gl::aptrText2, TOP_PROC> m_textCpu;
    std::array<psc::gl::aptrText2, TOP_PROC> m_textMem;
    psc::gl::aptrGeom2 createBox(GeometryContext *shaderContext, Gdk::RGBA &color);
    pProcess m_procRoot;
    constexpr static auto sdir = "/proc";
    guint m_size;
    TreeType m_treeType;
};
