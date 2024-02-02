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

#include "Processes.hpp"
#include "Diagram.hpp"
#include "LineShapeRenderer.hpp"
#include "SunDiscRenderer.hpp"
#include "FallShapeRenderer.hpp"


Processes::Processes(guint _size)
: m_procRoot{nullptr}
, m_size{_size}
, m_treeType{TREETYPE::ARC}
{
    resetProc();
    for (guint i = 0; i < ARRAYSIZE(m_memGeo); ++i) {
        m_memGeo[i] = nullptr;
        m_cpuGeo[i] = nullptr;
        m_textCpu[i] = nullptr;
        m_textMem[i] = nullptr;
    }
}


Processes::~Processes()
{
    resetProc();
    // as m_procRoot is a additional structure needs no cleanup (linking existing processes)
    for (auto p = mProcesses.begin(); p != mProcesses.end(); ) {
        pProcess proc = p->second;
        p = mProcesses.erase(p);      // make unreachable, the inc needs to be post (the doc says it is incremented internally this might be right in the case of a vector) !!!
        delete proc;                // delete
    }
    for (guint i = 0; i < ARRAYSIZE(m_memGeo); ++i) {
        if (m_memGeo[i] != nullptr)
            delete m_memGeo[i];
        if (m_cpuGeo[i] != nullptr)
            delete m_cpuGeo[i];
        if (m_textCpu[i] != nullptr)
            delete m_textCpu[i];
        if (m_textMem[i] != nullptr)
            delete m_textMem[i];
    }
}

void
Processes::resetProc()
{
   for (guint i = 0; i < ARRAYSIZE(pMem); ++i) {
        pMem[i] = nullptr;
    }
    for (guint i = 0; i < ARRAYSIZE(pCpu); ++i ) {
        pCpu[i] = nullptr;
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
    const char *sdir = "/proc";
    if ((dir = opendir(sdir)) != nullptr) {
        /* find all the processes in /proc */
        for (auto p = mProcesses.begin(); p != mProcesses.end(); ++p) {
            pProcess proc = p->second;
            if (proc->isActive()) {
                proc->setStage(Process::STAGE_RUN);
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
                        char path[64];
                        snprintf(path, sizeof(path), "%s/%ld", sdir, pid);
                        proc = new Process(path, pid, m_size);
                        mProcesses.insert(std::pair<long, pProcess>(pid, proc));
                    }
                    proc->update(cpu, mem);
                }
            }
        }
        for (auto p = mProcesses.begin(); p != mProcesses.end(); ) {
            pProcess proc = p->second;
            if (!proc->isTouched()) {   // if we didn't touch the entry process died
                if (proc->getStage() < Process::STAGE_CLS) {    // close in 2 steps to show status
                    proc->setStage(Process::STAGE_CLS);
                    ++p;
                }
                else {
                    p = mProcesses.erase(p);    // make unreachable, the inc needs to be post increment (the doc says it is incremented internally this might be right in the case of a vector) !!!
                    delete proc;            // delete
                }
            }
            else {
                ++p;
            }
        }
        closedir(dir);
    }
    findMax();
    buildTree();
}

void
Processes::buildTree()
{
    for (auto p = mProcesses.begin(); p != mProcesses.end(); ++p) {
        pProcess proc = p->second;      // as process assignments might change rebuild tree

        long ppid = proc->getPpid();
        auto pp = mProcesses.find(ppid);
        if (pp != mProcesses.end()) {
            pProcess pproc = pp->second;
            proc->setParent(pproc);
        }
        else {
            // some kernel internal porcess have ppid 0 dont expect much impact by them ?
            //std::cout << "Process " << ppid << " not founnd!" << std::endl;
        }
    }
    if (m_procRoot == nullptr) {
        auto pr = mProcesses.find(ROOT_PID);  // seems as we can be sure proc 1 is the root
        if (pr != mProcesses.end()) {
            m_procRoot = pr->second;
        }
        else {
            // as update and redraw happen asynchronously this will happen on first call
            std::cerr << "No process 1 to root proc tree!" << std::endl;
        }
    }
}

void
Processes::findMax()
{
    std::vector<pProcess> proc(mProcesses.size());

    for (auto p = mProcesses.begin(); p != mProcesses.end(); p++) {
        pProcess _proc = p->second;
        if (_proc != nullptr) {
            proc.push_back(_proc);
        }
    }
    std::sort(proc.begin(), proc.end(), CompareByMem());
    guint i = 0;
    //std::cout << "Memory" << std::endl;
    for (auto iter = proc.begin(); iter != proc.end(); ++iter) {
        pProcess pro = (*iter);
        //std::wcout << i << " " << pro->getDisplayName() << " " <<  pro->getMemUsage() << std::endl;
        if (pro->isActive()) {
            pMem[i++] = pro;
            if (i >= ARRAYSIZE(pMem)) {
                break;
            }
        }
    }
    std::sort(proc.begin(), proc.end(), CompareByCpu());
    i = 0;
    //std::cout << "Cpu" << std::endl;
    for (auto iter = proc.begin(); iter != proc.end(); ++iter) {
        pProcess pro = (*iter);
        //std::wcout << i << " " << pro->getDisplayName() << " " <<  pro->getCpuUsage() << " fl: " << pro->getCpuUsageBuf() << std::endl;
        if (pro->isActive()) {
            pCpu[i++] = pro;
            if (i >= ARRAYSIZE(pCpu)) {
                break;
            }
        }
    }
}

void
Processes::updateCpu(GraphShaderContext* pGraph_shaderContext,
	TextContext *_txtCtx, Font* pFont, std::shared_ptr<DiagramMonitor> cpu,
	Matrix &persView, Position &p)
{
    Gdk::RGBA colors[3];
    colors[0] = Gdk::RGBA("#a04000");   // working heat -> red
    colors[1] = Gdk::RGBA("#902000");
    colors[2] = Gdk::RGBA("#800000");
    if (m_cpuGeo[0] == nullptr) {
        for (guint i = 0; i < ARRAYSIZE(m_cpuGeo); ++i) {
            m_cpuGeo[i] = createBox(pGraph_shaderContext, colors[i]);
            m_cpuGeo[i]->setPosition(p);
            pGraph_shaderContext->addGeometry(m_cpuGeo[i]);
            m_textCpu[i] = _txtCtx->createText(pGraph_shaderContext, pFont);
            m_textCpu[i]->setScale(0.005f);
            Position p2(0.25f, 0.0f, 0.0f);
            m_textCpu[i]->setPosition(p2);
            m_cpuGeo[i]->addGeometry(m_textCpu[i]);
            p.y -= 0.3f;
        }
    }
    auto sum = std::make_shared<Buffer<double>>(m_size);
    for (guint i = 0; i < ARRAYSIZE(pCpu); ++i) {
        pProcess proc = pCpu[i];
        if (proc != nullptr) {
            //bool duplicat = FALSE;
            //for (int j = 0; j < ARRAYSIZE(pMem); ++j) {
            //    pProcess procMem = pMem[j];
            //    if (proc == procMem) {
            //        duplicat = TRUE;      // process is in both lists -> dont display list entry again
            //    }
            //}
            auto values = proc->getCpuData();
            sum->add(values);    // Stack graphs
            cpu->fill_buffers(cpu->getMonitor()->defaultValues()+i, sum, &colors[i]);
            //if (!duplicat) {
            Geometry *geo = m_cpuGeo[i];
            if (geo != nullptr) {
                //Matrix mvp = pGraph_shaderContext->setScalePos(persView, p, 1.0f);
                //geo->display(persView);
                Glib::ustring sname = proc->getDisplayName();
                m_textCpu[i]->setText(sname);
                //m_textCpu[i]->display(persView);
            }
            //}
        }
    }
}

void
Processes::updateMem(GraphShaderContext* pGraph_shaderContext,
	TextContext *_txtCtx, Font* pFont,
	std::shared_ptr<DiagramMonitor> mem, Matrix &persView, Position &p)
{
    Gdk::RGBA colors[3];
    colors[0] = Gdk::RGBA("#4000a0");   // mem -> blue
    colors[1] = Gdk::RGBA("#200090");
    colors[2] = Gdk::RGBA("#000080");
    if (m_memGeo[0] == nullptr) {
        for (guint i = 0; i < ARRAYSIZE(m_memGeo); ++i) {
            m_memGeo[i] = createBox(pGraph_shaderContext, colors[i]);
            m_memGeo[i]->setPosition(p);
            pGraph_shaderContext->addGeometry(m_memGeo[i]);
            m_textMem[i] = _txtCtx->createText(pGraph_shaderContext, pFont);
            m_textMem[i]->setScale(0.0045f);
            Position p2(0.25f, 0.0f, 0.0f);
            m_textMem[i]->setPosition(p2);
            m_memGeo[i]->addGeometry(m_textMem[i]);
            p.y -= 0.3f;
        }
    }

    auto sum = std::make_shared<Buffer<double>>(m_size);
    for (guint i = 0; i < ARRAYSIZE(pMem); ++i) {
        pProcess proc = pMem[i];
        if (proc != nullptr) {
            auto values = proc->getMemData();
            sum->add(values);    // stack graphs
            mem->fill_buffers(mem->getMonitor()->defaultValues()+i, sum, &colors[i]);
            Geometry *geo = m_memGeo[i];
            if (geo != nullptr) {
                //std::cout << "x " << x << " name " << proc->getName() << std::endl;
                //Matrix mvp = pGraph_shaderContext->setScalePos(persView, p, 1.0f);
                //geo->display(persView);
                std::ostringstream oss1;
                oss1 << proc->getDisplayName() << " ";
                double procMb = proc->getMemUsage() / 1024.0;
                oss1.precision(1);  // this shoud be sufficient
                oss1 << std::fixed <<  procMb << "M";
                Glib::ustring buffer = oss1.str();

                m_textMem[i]->setText(buffer);
                //m_textMem[i]->display(persView);
            }
        }
    }
}

void
Processes::setTreeType(const Glib::ustring &uProcessType)
{
    TREETYPE treeType = Processes::fromString(uProcessType);
    for (auto p : mProcesses) {    // delete previous geometries
        pProcess proc = p.second;
        proc->removeGeometry();
    }
    m_treeType = treeType;
}

void
Processes::update(GraphShaderContext *pGraph_shaderContext, TextContext *_txtCtx, Font *pFont,
                        std::shared_ptr<DiagramMonitor> cpu, std::shared_ptr<DiagramMonitor> mem, Matrix &persView)
{
    Position p(1.5f, 4.3f, 0.0f);
    updateCpu(pGraph_shaderContext, _txtCtx, pFont, cpu, persView, p);
    updateMem(pGraph_shaderContext, _txtCtx, pFont, mem, persView, p);

    if (m_procRoot != nullptr) {
        Position pos(-5.0f, -4.0f, -2.5f);      // fallshape (left edge)
        TreeRenderer *treeRenderer = nullptr;
        switch (m_treeType) {
        case TREETYPE::ARC:
            pos = Position(0.0f, -5.0f, 0.0f);  // sundisc (center)
            treeRenderer = new SunDiscRenderer();
            break;
        case TREETYPE::BLOCK:
            treeRenderer = new FallShapeRenderer();
            break;
        case TREETYPE::LINE:
            treeRenderer = new LineShapeRenderer();
            break;
        }
        if (treeRenderer) {
            TreeGeometry *geo = treeRenderer->create(m_procRoot, pGraph_shaderContext, _txtCtx, pFont);
            delete treeRenderer;
            geo->setPosition(pos);
        }
        // as geometries are added to context they will be displayed automatically
        //m_procRoot->displayRecursive(persView, mvp);
    }
}

Geometry*
Processes::createBox(GeometryContext *shaderContext, Gdk::RGBA &color)
{
    Geometry *pGeometry = new Geometry(GL_TRIANGLES, shaderContext);

    pGeometry->setRemoveChildren(false);
    Color c(color.get_red(), color.get_green(), color.get_blue());
    pGeometry->addCube(0.07f, c);
    pGeometry->create_vao();
    checkError("Error create vao data (process)");

    return pGeometry;
}

void
Processes::restore()
{
    Position pos(0.0f);
    Rotational rot(0.0f, 0.0f, 0.0f);    // angels need (also set in geometry constr.) leads to expanded tree otherwise

    for (auto p = mProcesses.begin(); p != mProcesses.end(); ++p) {
        long pid = p->first;
        if (pid != ROOT_PID) {  // reset position for all but first
            pProcess proc = p->second;
            TreeGeometry *geo = proc->getTreeGeometry();
            if (geo != nullptr) {
                geo->setScale(1.0f);
                geo->setPosition(pos);
                geo->setRotation(rot);
            }
        }
    }
}