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

#include "GraphShaderContext.hpp"
#include "Process.hpp"
#include "Font.hpp"
#include "DiagramMonitor.hpp"
#include "Text.hpp"

static const long ROOT_PID = 1l;

enum class TREETYPE {
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
    void findMax();
    void update(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, Font *pFont, std::shared_ptr<DiagramMonitor> cpu, std::shared_ptr<DiagramMonitor> mem, Matrix &persView);
    void setTreeType(const Glib::ustring &uProcessType);
    void buildTree();
    void restore();


    static TREETYPE fromString(const Glib::ustring &str) {
        if (str == "b") {
            return TREETYPE::BLOCK;
        }
        if (str == "l") {
            return TREETYPE::LINE;
        }
        return TREETYPE::ARC;       // use some default
    }
    void printInfo();
protected:
    void updateCpu(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, Font *pFont, std::shared_ptr<DiagramMonitor> cpu, Matrix &persView, Position &p);
    void updateMem(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, Font *pFont, std::shared_ptr<DiagramMonitor> mem, Matrix &persView, Position &p);

private:
    std::map<long, pProcess> mProcesses;
    pProcess pMem[3];       // proc highest mem usage
    pProcess pCpu[3];       // proc highest cpu usage
    Geometry *m_memGeo[3];
    Geometry *m_cpuGeo[3];
    Text *m_textCpu[3];
    Text *m_textMem[3];
    Geometry *createBox(GeometryContext *shaderContext, Gdk::RGBA &color);
    TreeNode *m_procRoot;

    guint m_size;
    TREETYPE m_treeType;
};
