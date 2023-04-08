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

#include <GenericGlmCompat.hpp>
#include <vector>
#include <time.h>
#ifdef USE_GLES
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>	// correct according to https://registry.khronos.org/OpenGL/index_es.php#headers3
#endif
#include <string>
// as this is still experimental
//#define DEBUG 1

#include "GpuExtAmdCounter.hpp"

const std::string GpuExtAmdCounters::m_prefix{"GLExtAmd "};
std::shared_ptr<GpuExtAmdCounters> GpuExtAmdCounters::m_amdExtCounters;


/*
 * for raspi this seems the way to access video core counters
 * https://forums.raspberrypi.com/viewtopic.php?t=305952
 * https://docs.broadcom.com/doc/12358545
 *   even if we need to link the whole OGL lib, maybe there is an better option???
 * now also working for nouveau.
 * minor issue remaining exit message...
 */

GpuAmdCounter::GpuAmdCounter(GpuCounterGroup* group, GLuint counter)
: m_group{group}
, m_counterId{counter}
{
    char counterName[256];
    glGetPerfMonitorCounterStringAMD(m_group->getGroupId(), counter,
				 sizeof(counterName), NULL, counterName);
    m_name = counterName;
	// Determine the counter type
	glGetPerfMonitorCounterInfoAMD(m_group->getGroupId(), m_counterId,
									GL_COUNTER_TYPE_AMD, &m_type);
}

std::string
GpuAmdCounter::getName() const
{
	return m_name;
}

std::string
GpuAmdCounter::getFullName() const
{
	return GpuExtAmdCounters::m_prefix + m_group->getName() + " " + getName();
}

GLuint
GpuAmdCounter::getCounterId() const
{
	return m_counterId;
}

uint64_t
GpuAmdCounter::getUint64() const
{
	return m_valuint64;
}

void
GpuAmdCounter::setUint64(uint64_t valuint64)
{
	m_valuint64 = valuint64;
	m_intVal = true;
}

bool
GpuAmdCounter::isInt() const
{
	return m_intVal;
}

GLfloat
GpuAmdCounter::getFloat() const
{
	return m_valueFloat;
}

void
GpuAmdCounter::setFloat(GLfloat val)
{
	m_valueFloat = val;
	m_floatVal = true;
}

bool
GpuAmdCounter::isFloat() const
{
	return m_floatVal;
}

GpuCounterGroup*
GpuAmdCounter::getGroup()
{
	return m_group;
}

GLuint
GpuAmdCounter::getType() const
{
	return m_type;
}

GpuCounterGroup::GpuCounterGroup(GLuint group)
: m_group{group}
, m_selectedCounters()
, m_removeCounters()
{
	char groupName[256];
	glGetPerfMonitorGroupStringAMD(group, sizeof(groupName), NULL, groupName);
	m_name = groupName;
	int numCounters = 0;
	glGetPerfMonitorCountersAMD(group, &numCounters,
							 &m_maxActiveCounters, 0, NULL);
	#ifdef DEBUG
	std::cout << "Grp" << groupName
		      << " numCounters " << numCounters
		      << " maxActiveCounters" << m_maxActiveCounters << std::endl;
	#endif
	std::vector<GLuint> counterList(numCounters);
	glGetPerfMonitorCountersAMD(group, NULL, NULL,
								numCounters, &counterList[0]);
	for (int i = 0; i < numCounters; ++i) {
		auto cntId = counterList[i];
		auto cnt = std::make_shared<GpuAmdCounter>(this, cntId);
		m_counterIds.insert(std::pair<GLuint, std::shared_ptr<GpuAmdCounter>>(cntId, cnt));
		auto cntName = cnt->getName();
		m_counterNames.insert(std::pair<std::string, std::shared_ptr<GpuAmdCounter>>(cntName, cnt));
	}
}

std::shared_ptr<GpuAmdCounter>
GpuCounterGroup::get(const std::string& name)
{
	auto entry = m_counterNames.find(name);
	if (entry != m_counterNames.end()) {
		auto cnt = entry->second;
		return cnt;
	}
	return nullptr;
}

std::shared_ptr<GpuAmdCounter>
GpuCounterGroup::get(GLuint counterId)
{
	auto entry = m_counterIds.find(counterId);
	if (entry != m_counterIds.end()) {
		auto cnt = entry->second;
		return cnt;
	}
	return nullptr;
}

std::string
GpuCounterGroup::getName() const
{
	return m_name;
}

GLuint
GpuCounterGroup::getGroupId() const
{
	return m_group;
}

std::list<std::string>
GpuCounterGroup::list()
{
	std::list<std::string> ret;
	for (auto entry : m_counterIds) {
		auto cnt = entry.second;
		ret.push_back(cnt->getFullName());
	}
	return ret;
}

std::vector<GLuint>
GpuCounterGroup::select(GLuint counterId, GLboolean select)
{
	if (select) {
		bool add = true;
		for (auto selCnt : m_selectedCounters) {
			if (selCnt == counterId) {
				add = false;
			}
		}
		if (add) {
			m_selectedCounters.push_back(counterId);
		}
	}
	else {
		m_removeCounters.push_back(counterId);
		for (auto iter = m_selectedCounters.begin(); iter != m_selectedCounters.end(); ) {
			if (*iter == counterId) {
				iter = m_selectedCounters.erase(iter);
			}
			else {
				++iter;
			}
		}
	}
	return m_selectedCounters;
}

std::vector<GLuint>
GpuCounterGroup::getAndClearSelectedCounters()
{
	auto selectedCounter = m_selectedCounters;
	m_selectedCounters.clear();
	return selectedCounter;
}

std::vector<GLuint>
GpuCounterGroup::getAndClearRemoveCounters()
{
	auto removeCounters = m_removeCounters;
	m_removeCounters.clear();
	return removeCounters;
}

GpuExtAmdCounter::GpuExtAmdCounter(std::shared_ptr<GpuAmdCounter>& counter
	,std::shared_ptr<GpuAmdExtMonitor>& monitor)
: GpuCounter(counter->getFullName())
, m_counter{counter}
, m_monitor{monitor}
{
	#ifdef DEBUG
	std::cout << "GpuExtAmdCounter::GpuExtAmdCounter name " << m_counter->getName() << std::endl;
	#endif
	m_counter->getGroup()->select(m_counter->getCounterId(), GL_TRUE);
}

GpuExtAmdCounter::~GpuExtAmdCounter()
{
	m_counter->getGroup()->select(m_counter->getCounterId(), GL_FALSE);
}

std::string
GpuExtAmdCounter::get_unit()
{
	if (m_counter)
		return "/s"; // as name is too bulky
	return "";
}

GLuint
GpuExtAmdCounter::update(Buffer<guint64>& hist, guint64 dt, int refresh_rate)
{
	#ifdef DEBUG
	std::cout << "GpuExtAmdCounter::update name " << m_counter->getName() << std::endl;
	#endif
    if (!m_counter) {
        std::cerr << "No counters identified!" << std::endl;
        return 0;
    }
    if (m_counter->isInt()) {
        uint64_t cycle = m_counter->getUint64();
        #ifdef DEBUG
        std::cout << "Cycle int " << cycle << " dt " << dt << std::endl;
        #endif
        guint64 cyclesElapsedL = cycle * 1e6l / dt;    // show always per sec
        hist.set(cyclesElapsedL);
        return cycle;
    }
    else if (m_counter->isFloat()) {
        GLfloat cycle = m_counter->getFloat();
        #ifdef DEBUG
        std::cout << "Cycle fl " << cycle << std::endl;
        #endif
        GLfloat cyclesElapsedL = cycle * 1.e6f / (float)dt;    // show always per sec
        hist.set(cyclesElapsedL);
        return cycle;
    }
	#ifdef DEBUG
	std::cout << "GpuExtAmdCounter::update no value " << std::endl;
	#endif
    return 0;
}

GpuExtAmdCounters::GpuExtAmdCounters()
: GpuCounters::GpuCounters()
, m_groups()
{
	checkGroups();
}

GpuExtAmdCounters::~GpuExtAmdCounters()
{
}

bool
GpuExtAmdCounters::isGLExtension(const char* ext)
{
	#ifdef DEBUG
	const char *vendor = (const char*)glGetString(GL_VENDOR);
	if (vendor != nullptr) {
		std::cerr << "Vendor " << vendor << std::endl;
	}
	const char *renderer = (const char*)glGetString(GL_RENDERER);
	if (renderer != nullptr) {
		std::cerr << "Renderer " << renderer << std::endl;
	}
	const char *version = (const char*)glGetString(GL_VERSION);
	if (version != nullptr) {
		std::cerr << "Version " << version << std::endl;
	}
	const char *shading = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if (shading != nullptr) {
		std::cerr << "Shading " << shading << std::endl;
	}
	#endif
	// hope that one of these methods works...(as there seems to be no preferable)
	const char *exts = (const char*)glGetString(GL_EXTENSIONS);
	if (exts != nullptr) {
		return (strstr(exts, ext) != nullptr);
	}
	else {
		GLint n = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &n);
		for (GLint i = 0; i < n; i++) {
			const char* exts = (const char*)glGetStringi(GL_EXTENSIONS, i);
			#ifdef DEBUG
			std::cout << i << "extensions " << exts << std::endl;
			#endif
			if (strstr(exts, ext) != nullptr) {
				return true;
			}
		}
	}
	return false;
}

void
GpuExtAmdCounters::checkGroups()
{
	if (isGLExtension("GL_AMD_performance_monitor")) {
		GLint n = 0;
		glGetPerfMonitorGroupsAMD(&n, 0, NULL);
		#ifdef DEBUG
		std::cout << "GpuExtAmdCounters::list n " << n << std::endl;
		#endif
		if (m_groups.empty()
		 && n > 0) {
			std::vector<GLuint> groups(n);
			glGetPerfMonitorGroupsAMD(NULL, n, &groups[0]);
			for (GLint i = 0; i < n; ++i) {
				auto grp = std::make_shared<GpuCounterGroup>(groups[i]);
				m_groups.push_back(grp);
			}
		}
    }
	else {
        std::cerr << "Amd counter extensions not supported." << std::endl;
	}
}

std::list<std::string>
GpuExtAmdCounters::list()
{
    #ifdef DEBUG
	std::cout << "GpuExtAmdCounters::list m_groups " << m_groups.size() << std::endl;
    #endif
	std::list<std::string> ret;
	for (auto grp : m_groups) {
		for (auto cntName : grp->list()) {
			ret.push_back(cntName);
		}
	}
	return ret;
}

std::shared_ptr<GpuCounter>
GpuExtAmdCounters::get(const std::string& name)
{
    if (name.rfind(m_prefix, 0) == 0) {
        for (auto group : m_groups) {
			auto groupName = group->getName();
            if (name.rfind(groupName, m_prefix.length()) == m_prefix.length()) {  // works only for ascii names
                auto remain = name.substr(m_prefix.length() + groupName.length() + 1);
                auto counter = group->get(remain);
                if (counter) {
					if (!m_monitor) {
						m_monitor = std::make_shared<GpuAmdExtMonitor>(this);
					}

					#ifdef DEBUG
					std::cout << "GpuExtAmdCounters::get name " << name << std::endl;
					#endif
                    return std::make_shared<GpuExtAmdCounter>(counter, m_monitor);
                }
            }
        }
        std::cerr << "Expected to find name " << name << "in GpuExtAmdCounters::get" << std::endl;
    }
    return nullptr;
}

void
GpuExtAmdCounters::read()
{
	if (m_monitor) {	// if non was requested do nothing
		m_monitor->read();
	}
}

std::shared_ptr<GpuExtAmdCounters>
GpuExtAmdCounters::create()
{
    if (!m_amdExtCounters) {
        m_amdExtCounters = std::make_shared<GpuExtAmdCounters>();
    }
    return m_amdExtCounters;
}

void
GpuExtAmdCounters::reset()
{
	m_amdExtCounters.reset();
}

std::shared_ptr<GpuCounterGroup>
GpuExtAmdCounters::getGroup(GLuint groupId) const
{
	for (auto grp : m_groups) {
		if (grp->getGroupId() == groupId) {
			return grp;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<GpuCounterGroup>>
GpuExtAmdCounters::getAllGroups() const
{
	return m_groups;
}

GpuAmdExtMonitor::GpuAmdExtMonitor(GpuExtAmdCounters* counters)
: m_counters{counters}
{
}

GpuAmdExtMonitor::~GpuAmdExtMonitor()
{
	if (m_monitor > 0) {
		if (m_queryActive) {
			endQuery();
		}
		glDeletePerfMonitorsAMD(1, &m_monitor);
		m_monitor = 0;
	}
}

bool
GpuAmdExtMonitor::isResultAvailable()
{
	GLuint resultAvailable = 1u, queryCnt = 0u;
	while(queryCnt < 1000) {    // ensure result is available, but not indefinitely
		glGetPerfMonitorCounterDataAMD(m_monitor, GL_PERFMON_RESULT_AVAILABLE_AMD,
									sizeof(GLuint), &resultAvailable, NULL);
		if (resultAvailable) {
			return true;
		}
		struct timespec req;// yield as it takes time for nouvea x64 get 5..10 cycles
		req.tv_nsec = 1e3L; // 1e3 nanos = 1usec
		req.tv_sec = 0;
		nanosleep(&req, nullptr);
	}
	return false;
}


//struct CountRead {
//	GLuint groupId;
//	GLuint counterId;
//	union {
//		uint64_t readUint64;
//		GLfloat readFloat;
//      GLuint readUint32;
//	};
//};
void
GpuAmdExtMonitor::read()
{
	if (m_monitor > 0) {
		if (m_queryActive) {
			#ifdef DEBUG
			std::cout << "GpuAmdExtMonitor::read" << std::endl;
			#endif
			endQuery();
			isResultAvailable();
			// read the counters
			GLuint resultSizeBytes = 0;
			glGetPerfMonitorCounterDataAMD(m_monitor, GL_PERFMON_RESULT_SIZE_AMD,
										   sizeof(GLuint), &resultSizeBytes, NULL);

			#ifdef DESTRUCT
			std::cout << "monitor " << m_monitor << " resultSize " << resultSizeBytes << std::endl;
			#endif
			if (resultSizeBytes > 0) {	// at the moment that shoud exclude invalid stuff
				std::vector<GLuint> counterData((resultSizeBytes / sizeof(GLuint))+1);	// increase by one to avoid missing for non int aligned size

				GLsizei bytesWritten;
				glGetPerfMonitorCounterDataAMD(m_monitor, GL_PERFMON_RESULT_AMD,
											   resultSizeBytes, &counterData[0], &bytesWritten);
				#ifdef DEBUG
				printf("monitor %d bytesWritten %d\n", m_monitor, bytesWritten);
				#endif
				GLsizei wordCountWritten = bytesWritten / sizeof(GLuint);
				// display or log counter info
				GLsizei wordCount = 0;
				while (wordCount < wordCountWritten) {
					GLuint groupId = counterData[wordCount++];
					GLuint counterId = counterData[wordCount++];
					std::shared_ptr<GpuAmdCounter> counter;
					auto grp = m_counters->getGroup(groupId);
					if (grp) {
						counter = grp->get(counterId);
						if (counter) {
                            bool exit = false;
                            auto counterType = counter->getType();
                            switch (counterType) {
                            case GL_UNSIGNED_INT64_AMD: {
								uint64_t counterResult = *(uint64_t*)(&counterData[wordCount]);
								counter->setUint64(counterResult);
								wordCount += 2;     // unit is GLuint, uint64_t is 2 of these
                                }
                                break;
                            case GL_UNSIGNED_INT: {
								GLuint counterResult = (GLuint)(counterData[wordCount]);
								counter->setUint64(counterResult);
								wordCount += 1;     // unit is GLuint, uint32_t is 1 of these
                                }
                                break;
                            case GL_FLOAT: {
								float counterResult = (float)(counterData[wordCount]);
								counter->setFloat(counterResult);
								wordCount += 1;     // unit is GLuint, float is 1 of these
                                }
                                break;
                            case GL_PERCENTAGE_AMD: {
								float counterResult = (float)(counterData[wordCount]);
								counter->setFloat(counterResult);   // is float as it seems
								wordCount += 1;     // unit is GLuint, percent is 1 of these
                                // guessed from https://cpp.hotexamples.com/examples/-/-/glEndPerfMonitorAMD/cpp-glendperfmonitoramd-function-examples.html
                                //float range[2]; // here we get for percent 0...100 but values are 131 which get back into range when used per second so use range for max?
                                //glGetPerfMonitorCounterInfoAMD(groupId, counterId,
                                //     GL_COUNTER_RANGE_AMD, range);
                                //#ifdef DEBUG
                                //std::cout << "Read percent " << counterResult << " min " << range[0] << " max " << range[1] << std::endl;
                                //#endif
                                }
                                break;
							// else if ( ... ) check for other counter types
                            default:
								std::cerr << "GpuMonitor not yet handled type " << counter->getType()
										  << ". Stop reading as we don't know how to handle rest!"
										  << std::endl;
                                exit = false;
								break;
                            }
                            if (exit) {
                                break;
                            }
						}
					}
					else {
						std::cerr << "Cound not identify group " << groupId << std::endl;
					}
				}
			}
		}
	}
	if (m_monitor == 0) {
		glGenPerfMonitorsAMD(1, &m_monitor);
	}
	// do select synchronously to query end/begin
	for (auto grp : m_counters->getAllGroups()) {
		auto removeCounters = grp->getAndClearRemoveCounters();
		#ifdef DESTRUCT
		std::cout << "GpuAmdExtMonitor remove count " << removeCounters.size() << std::endl;
		#endif
		if (!removeCounters.empty()) {
			glSelectPerfMonitorCountersAMD(m_monitor, GL_FALSE, grp->getGroupId(), removeCounters.size(), &removeCounters[0]);
		}
		auto selectedCounters = grp->getAndClearSelectedCounters();
		#ifdef DESTRUCT
		std::cout << "GpuAmdExtMonitor select count " << selectedCounters.size() << std::endl;
		#endif
		if (!selectedCounters.empty()) {
			glSelectPerfMonitorCountersAMD(m_monitor, GL_TRUE, grp->getGroupId(), selectedCounters.size(), &selectedCounters[0]);
		}
	}
	beginQuery();
}

bool
GpuAmdExtMonitor::isQueryActive()
{
	return m_queryActive;
}

GLuint
GpuAmdExtMonitor::getMonitor() const
{
	return m_monitor;
}

void
GpuAmdExtMonitor::beginQuery()
{
	if (!m_queryActive) {
		#ifdef DEBUG
		std::cout << "GpuAmdExtMonitor::beginQuery " << m_monitor << std::endl;
		#endif
		glBeginPerfMonitorAMD(m_monitor);
		m_queryActive= true;
	}
}

void
GpuAmdExtMonitor::endQuery()
{
	if (m_queryActive) {
		#ifdef DEBUG
		std::cout << "GpuAmdExtMonitor::endQuery " << m_monitor << std::endl;
		#endif
		glEndPerfMonitorAMD(m_monitor);
		m_queryActive = false;
	}
}
