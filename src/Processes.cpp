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

#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <Log.hpp>

#include "Processes.hpp"
#include "TreeNode2.hpp"
#include "Diagram.hpp"
#include "LineShapeRenderer2.hpp"
#include "SunDiscRenderer2.hpp"
#include "FallShapeRenderer2.hpp"


Processes::Processes(uint32_t _size)
: m_size{_size}
, m_treeType{TreeType::ARC}
{
}


Processes::~Processes()
{
    resetProc();
    // as m_procRoot is a additional structure needs no cleanup (linking existing processes)
    for (uint32_t i = 0; i < m_memGeo.size(); ++i) {
        m_memGeo[i].reset();
        m_cpuGeo[i].reset();
        m_textCpu[i].reset();
        m_textMem[i].reset();
    }
    mProcesses.clear();
}

void
Processes::resetProc()
{
   for (auto& topMem : m_topMem) {
        if (topMem) {
            topMem.reset();
        };
    }
    for (auto& topCpu : m_topCpu) {
        if (topCpu) {
            topCpu.reset();
        }
    }
}

void
Processes::printInfo()
{
	unsigned long sumMemUsage = 0;
	unsigned long sumMemGraph = 0;

	for (auto p : mProcesses) {
		pProcess proc = p.second;
		sumMemUsage += proc->getMemUsage();
		sumMemGraph += proc->getMemGraph();
	}
	std::cout <<  "all process memUsage " << sumMemUsage << std::endl;
	std::cout <<  "all process memGraph " << sumMemGraph << std::endl;
}

void
Processes::update(std::shared_ptr<Monitor> cpu, std::shared_ptr<Monitor> mem)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(sdir)) != nullptr) {
        /* find all the processes in /proc */
        for (auto& p : mProcesses) {
            auto& proc = p.second;
            if (proc->isActive()) {
                proc->setStage(psc::gl::TreeNodeState::Running);
            }
            proc->setTouched(false);
        }
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR) {
                long pid = atol(ent->d_name);
                if (pid > 0) {
                    auto p = mProcesses.find(pid);
                    pProcess proc;
                    if (p != mProcesses.end()) {
                        proc = p->second;
                    }
                    else {
                        auto path = Glib::ustring::sprintf("%s/%ld", sdir, pid);
                        proc = std::make_shared<Process>(path, pid, m_size);
                        mProcesses.insert(std::pair(pid, proc));
                    }
                    proc->update(cpu, mem);
                }
            }
        }
        closedir(dir);
        for (auto p = mProcesses.begin(); p != mProcesses.end(); ) {
            auto& proc = p->second;
            if (!proc->isTouched()) {   // if we didn't touch the entry process died
                if (proc->getStage() < psc::gl::TreeNodeState::Close) {    // close in 2 steps to show status
                    proc->setStage(psc::gl::TreeNodeState::Close);
                    ++p;
                }
                else {
                    auto parent = proc->getParent();
                    if (parent) {   // remove any reference
                        parent->remove(proc.get());
                    }
                    p = mProcesses.erase(p);    // make unreachable, the inc needs to be post increment (the doc says it is incremented internally this might be right in the case of a vector) !!!
                }
            }
            else {
                ++p;
            }
        }
    }
    findMax(m_topMem, m_topCpu);
    buildTree();
}

void
Processes::buildTree()
{
    for (auto& p : mProcesses) {
        auto& proc = p.second;      // as process assignments might change rebuild tree

        long ppid = proc->getPpid();
        auto pp = mProcesses.find(ppid);
        if (pp != mProcesses.end()) {
            pProcess pproc = pp->second;
            proc->setParent(pproc.get(), proc);
        }
        else {
            // some kernel internal porcess have ppid 0 dont expect much impact by them ?
            //std::cout << "Process " << ppid << " not founnd!" << std::endl;
        }
    }
    if (!m_procRoot) {
        auto pr = mProcesses.find(ROOT_PID);  // seems as we can be sure proc 1 is the root
        if (pr != mProcesses.end()) {
            m_procRoot = pr->second;
        }
        else {
            // as update and redraw happen asynchronously this will happen on first call
	    psc::log::Log::logNow(psc::log::Level::Error, "No process 1 to root proc tree!");
        }
    }
}

void
Processes::findMax(std::array<pProcess, TOP_PROC>& topMem
                  ,std::array<pProcess, TOP_PROC>& topCpu)
{
    std::vector<pProcess> procs(mProcesses.size());
    for (auto& p : mProcesses) {
        auto proc = p.second;
        if (proc && proc->isActive()) {
            procs.push_back(proc);
        }
    }
    std::sort(procs.begin(), procs.end(), CompareByMem());
    uint32_t i = 0;
    //std::cout << "Memory" << std::endl;
    for (auto& pro : procs) {
        //std::wcout << i << " " << pro->getDisplayName() << " " <<  pro->getMemUsage() << std::endl;
        topMem[i++] = pro;
        if (i >= topMem.size()) {
            break;
        }
    }
    std::sort(procs.begin(), procs.end(), CompareByCpu());
    i = 0;
    //std::cout << "Cpu" << std::endl;
    for (auto& pro : procs) {
        //std::wcout << i << " " << pro->getDisplayName() << " " <<  pro->getCpuUsage() << " fl: " << pro->getCpuUsageBuf() << std::endl;
        topCpu[i++] = pro;
        if (i >= topCpu.size()) {
            break;
        }
    }
}

void
Processes::updateCpu(GraphShaderContext* pGraph_shaderContext,
	TextContext *_txtCtx, const psc::gl::ptrFont2& pFont, std::shared_ptr<DiagramMonitor> cpu,
	Matrix &persView, Position &p)
{
    Gdk::RGBA colors[3];
    colors[0] = Gdk::RGBA("#a04000");   // working heat -> red
    colors[1] = Gdk::RGBA("#902000");
    colors[2] = Gdk::RGBA("#800000");
    if (!m_cpuGeo[0]) {
        for (uint32_t i = 0; i < Processes::TOP_PROC; ++i) {
            m_cpuGeo[i] = createBox(pGraph_shaderContext, colors[i]);
            auto lcpuGeo = m_cpuGeo[i].lease();
            if (lcpuGeo) {
                lcpuGeo->setPosition(p);
                //pGraph_shaderContext->addGeometry(m_cpuGeo[i]);
                m_textCpu[i] = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, pGraph_shaderContext, pFont);
                auto ltextCpu = m_textCpu[i].lease();
                if (ltextCpu) {
                    ltextCpu->setTextContext(_txtCtx);
                    ltextCpu->setScale(0.005f);
                    Position p2(0.25f, 0.0f, 0.0f);
                    ltextCpu->setPosition(p2);
                }
                lcpuGeo->addGeometry(m_textCpu[i]);
            }
            p.y -= 0.3f;
        }
    }
    auto sum = std::make_shared<Buffer<double>>(m_size);
    for (uint32_t i = 0; i < Processes::TOP_PROC; ++i) {
        auto proc = m_topCpu[i];
        if (proc) {
            //bool duplicat = FALSE;
            //for (int j = 0; j < Processes::TOP_PROC; ++j) {
            //    pProcess procMem = pMem[j];
            //    if (proc == procMem) {
            //        duplicat = TRUE;      // process is in both lists -> dont display list entry again
            //    }
            //}
            auto values = proc->getCpuData();
            sum->add(values);    // Stack graphs
            cpu->fill_buffers(cpu->getMonitor()->defaultValues()+i, sum, &colors[i]);
            //if (!duplicat) {
            auto geo = m_cpuGeo[i];
            if (geo) {
                //Matrix mvp = pGraph_shaderContext->setScalePos(persView, p, 1.0f);
                //geo->display(persView);
                Glib::ustring sname = proc->getDisplayName();
                auto ltxt = m_textCpu[i].lease();
                if (ltxt) {
                    ltxt->setText(sname);
                }
                //m_textCpu[i]->display(persView);
            }
            //}
        }
    }
}

void
Processes::updateMem(GraphShaderContext* pGraph_shaderContext,
	TextContext *_txtCtx, const psc::gl::ptrFont2& pFont,
	std::shared_ptr<DiagramMonitor> mem, Matrix &persView, Position &p)
{
    Gdk::RGBA colors[Processes::TOP_PROC];
    colors[0] = Gdk::RGBA("#4000a0");   // mem -> blue
    colors[1] = Gdk::RGBA("#200090");
    colors[2] = Gdk::RGBA("#000080");
    if (!m_memGeo[0]) {
        for (uint32_t i = 0; i < Processes::TOP_PROC; ++i) {
            m_memGeo[i] = createBox(pGraph_shaderContext, colors[i]);
            auto lmemGeo = m_memGeo[i].lease();
            if (lmemGeo) {
                lmemGeo->setPosition(p);
                //pGraph_shaderContext->addGeometry(m_memGeo[i]);
                m_textMem[i] = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, pGraph_shaderContext, pFont);
                auto lTextMem = m_textMem[i].lease();
                if (lTextMem) {
                     lTextMem->setTextContext(_txtCtx);
                    lTextMem->setScale(0.0045f);
                    Position p2(0.25f, 0.0f, 0.0f);
                    lTextMem->setPosition(p2);
                }
                lmemGeo->addGeometry(m_textMem[i]);
            }
            p.y -= 0.3f;
        }
    }

    auto sum = std::make_shared<Buffer<double>>(m_size);
    for (uint32_t i = 0; i < Processes::TOP_PROC; ++i) {
        pProcess proc = m_topMem[i];
        if (proc) {
            auto values = proc->getMemData();
            sum->add(values);    // stack graphs
            mem->fill_buffers(mem->getMonitor()->defaultValues()+i, sum, &colors[i]);
            auto geo = m_memGeo[i];
            if (geo) {
                //std::cout << "x " << x << " name " << proc->getName() << std::endl;
                //Matrix mvp = pGraph_shaderContext->setScalePos(persView, p, 1.0f);
                //geo->display(persView);
                double procMb = proc->getMemUsage() / 1024.0;
                auto buffer = Glib::ustring::sprintf("%s %.1lfM", proc->getDisplayName(), procMb);
                auto ltxtMem = m_textMem[i].lease();
                if (ltxtMem) {
                    ltxtMem->setText(buffer);
                }
                //m_textMem[i]->display(persView);
            }
        }
    }
}

void
Processes::setTreeType(const Glib::ustring &uProcessType)
{
    TreeType treeType = Processes::fromString(uProcessType);
    for (auto p : mProcesses) {    // delete previous geometries
        pProcess proc = p.second;
        proc->removeGeometry();
    }
    m_treeType = treeType;
}

void
Processes::display(
            GraphShaderContext *pGraph_shaderContext
          , TextContext *_txtCtx
          , const psc::gl::ptrFont2& pFont
          , std::shared_ptr<DiagramMonitor> cpu
          , std::shared_ptr<DiagramMonitor> mem
          , Matrix &persView)
{
    Position p(1.5f, 4.3f, 0.0f);
    updateCpu(pGraph_shaderContext, _txtCtx, pFont, cpu, persView, p);
    updateMem(pGraph_shaderContext, _txtCtx, pFont, mem, persView, p);
    displayTops(pGraph_shaderContext, persView);

    if (m_procRoot) {
        Position pos(-5.0f, -4.0f, -2.5f);      // fallshape (left edge)
        std::shared_ptr<psc::gl::TreeRenderer2> treeRenderer;
        switch (m_treeType) {
        case TreeType::ARC:
            pos = Position(0.0f, -5.0f, 0.0f);  // sundisc (center)
            treeRenderer = std::make_shared<psc::gl::SunDiscRenderer2>();
            break;
        case TreeType::BLOCK:
            treeRenderer = std::make_shared<psc::gl::FallShapeRenderer2>();
            break;
        case TreeType::LINE:
            treeRenderer = std::make_shared<psc::gl::LineShapeRenderer2>();
            break;
        }
        if (treeRenderer) {
            auto geo = treeRenderer->create(m_procRoot, pGraph_shaderContext, _txtCtx, pFont);
            auto lgeo = geo.lease();
            if (lgeo) {
                lgeo->setPosition(pos);
                //std::cout << "Processes display tree start" << std::endl;
                lgeo->display(pGraph_shaderContext, persView);
                //std::cout << "Processes display tree end" << std::endl;
            }
            else {
	         psc::log::Log::logNow(psc::log::Level::Error, "No tree root to display!");
            }
        }
        else {
	    psc::log::Log::logNow(psc::log::Level::Error, Glib::ustring::sprintf("Unknown tree renderer %d", static_cast<std::underlying_type<TreeType>::type>(m_treeType)));
                     
        }
        // as geometries are added to context they will be displayed automatically
        //m_procRoot->displayRecursive(persView, mvp);
    }
}

psc::gl::aptrGeom2
Processes::createBox(GeometryContext *shaderContext, Gdk::RGBA &color)
{
    auto pGeometry = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, shaderContext);
    auto lGeom = pGeometry.lease();
    if (lGeom) {
        lGeom->setRemoveChildren(false);
        Color c(color.get_red(), color.get_green(), color.get_blue());
        lGeom->addCube(0.07f, c);
        lGeom->create_vao();
        checkError("Error create vao data (process)");
    }
    return pGeometry;
}

void
Processes::restore()
{
    Position pos(0.0f);
    Rotational rot(0.0f, 0.0f, 0.0f);    // angels need (also set in geometry constr.) leads to expanded tree otherwise

    for (auto& p : mProcesses) {
        auto pid = p.first;
        if (pid != ROOT_PID) {  // reset position for all but first
            auto& proc = p.second;
            auto geo = proc->getTreeGeometry();
            if (geo) {
                auto lgeo = geo.lease();
                if (lgeo) {
                    lgeo->setScale(1.0f);
                    lgeo->setPosition(pos);
                    lgeo->setRotation(rot);
                }
            }
        }
    }
}

void
Processes::displayTops(NaviContext* context, const Matrix &projView)
{
    for (auto& mem : m_memGeo) {
        auto lmem = mem.lease();
        if (lmem) {
            lmem->display(context, projView);
        }
    }
    for (auto& cpu : m_cpuGeo) {
        auto lcpu = cpu.lease();
        if (lcpu) {
            lcpu->display(context, projView);
        }
    }
}
