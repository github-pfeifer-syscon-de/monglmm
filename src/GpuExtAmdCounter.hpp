/*
 * Copyright (C) 2022 PPf <gnu3@pfeifer-syscon.de>
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

#include "HistMonitor.hpp"
#include "GpuCounter.hpp"

// These classes encapsulate the  glGetPerfMonitorXXXAMD GL extension
class GpuCounterGroup;
class GpuExtAmdCounter;
class GpuExtAmdCounters;

class GpuAmdCounter {
public:
    GpuAmdCounter(GpuCounterGroup* group, GLuint counter);
    std::string getName() const;
    std::string getFullName() const;
    GLuint getCounterId() const;
    uint64_t getUint64() const;
    void setUint64(uint64_t val);
    bool isInt() const;
    GLfloat getFloat() const;
    void setFloat(GLfloat val);
    bool isFloat() const;
    GpuCounterGroup* getGroup();
    GLuint getType() const;
private:
    GpuCounterGroup* m_group;
    GLuint m_counterId;
    std::string m_name;
    uint64_t m_valuint64{0l};
    bool m_intVal{false};
    GLfloat m_valueFloat{0.0f};
    bool m_floatVal{false};
    GLuint m_type{0u};
};


class GpuCounterGroup {
public:
    GpuCounterGroup(GLuint group);
    std::shared_ptr<GpuAmdCounter> get(const std::string& name);
    std::shared_ptr<GpuAmdCounter> get(GLuint counterId);
    GLuint getGroupId() const;
    std::string getName() const;
    std::string getFullName() const;
    std::list<std::string> list();
    std::vector<GLuint> select(GLuint counter, GLboolean select);
    std::vector<GLuint> getAndClearSelectedCounters();
    std::vector<GLuint> getAndClearRemoveCounters();
protected:
private:
    GLuint m_group;
    GpuExtAmdCounters* m_counters;
    std::map<GLuint, std::shared_ptr<GpuAmdCounter>> m_counterIds;
    std::map<std::string, std::shared_ptr<GpuAmdCounter>> m_counterNames;
    std::string m_name;
    int m_maxActiveCounters {0};
    std::vector<GLuint> m_selectedCounters;
    std::vector<GLuint> m_removeCounters;
};

// encapsulate the monitor functions
class GpuAmdExtMonitor {
public:
    GpuAmdExtMonitor(GpuExtAmdCounters* counters);
    virtual ~GpuAmdExtMonitor();
    GLuint getMonitor() const;
    bool isQueryActive();
    void beginQuery();
    void endQuery();
    void read();
    bool isResultAvailable();
private:
    bool m_queryActive{false};
    GLuint m_monitor{0};
    const GpuExtAmdCounters* m_counters;
};


class GpuExtAmdCounter : public GpuCounter {
public:
    GpuExtAmdCounter(std::shared_ptr<GpuAmdCounter>& counter, std::shared_ptr<GpuAmdExtMonitor>& monitor);
    virtual ~GpuExtAmdCounter();
    std::string get_unit();

    GLuint update(Buffer<guint64>& hist, guint64 dt, int refresh_rate) override;
protected:

private:
    std::shared_ptr<GpuAmdCounter> m_counter;
    std::shared_ptr<GpuAmdExtMonitor> m_monitor;
};

class GpuExtAmdCounters : public GpuCounters {
public:
    GpuExtAmdCounters();
    virtual ~GpuExtAmdCounters();

    std::list<std::string> list() override;
    std::shared_ptr<GpuCounter> get(const std::string& name) override;
    std::shared_ptr<GpuCounterGroup> get(const char* group);
    std::shared_ptr<GpuCounterGroup> getGroup(GLuint groupId) const;
    void read();
    static std::shared_ptr<GpuExtAmdCounters> create();
    static void reset();
    std::vector<std::shared_ptr<GpuCounterGroup>> getAllGroups() const;
    static const std::string m_prefix;
protected:
    bool isGLExtension(const char* ext);
    void checkGroups();
private:
    static std::shared_ptr<GpuExtAmdCounters> m_amdExtCounters;
    std::vector<std::shared_ptr<GpuCounterGroup>> m_groups;
    std::shared_ptr<GpuAmdExtMonitor> m_monitor;
};
