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

#include <stdio.h>
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
#include <cpuid.h>
#endif
#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <cstdlib>
#include <errno.h>


#include "G15Worker.hpp"
#include "ClkMonitor.hpp"
#include <FileByLine.hpp>

using namespace std;


ClkMonitor::ClkMonitor(guint points)
: Monitor(points, "Clk")
, cpus{0}
, maxMHz{1.0}
{
    m_enabled = false;          // disabled by default
    for (guint i = 0; i < CLOCKS; ++i) {
        clkMHz[i] = 0.0;
        colors[i] = nullptr;
        for (guint j = 0; j < CLOCKS; ++j) {
            m_clockStep[i][j] = 0;
            m_clockTime[i][j] = 0;
        }
    }
}

ClkMonitor::~ClkMonitor()
{
    for (guint i = 0; i < CLOCKS; ++i) {
        Gdk::RGBA *rgb = colors[i];
        if (rgb != nullptr) {
            delete rgb;
        }
        colors[i] = nullptr;
    }
}

unsigned long
ClkMonitor::getTotal() {
    return maxMHz;
}

Gtk::Box *
ClkMonitor::create_config_page(MonglView *monglView)
{
    auto hw_box = create_default_config_page(
            _("Display Clk usage"),
            _("Clk prim"),
            _("Clk sec"));

    return hw_box;
}

// this coud also be read from /proc/cpuinfo, use lscpu
/*
processor	: 0
model name	: ARMv7 Processor rev 3 (v7l)
BogoMIPS	: 270.00
Features	: half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 lpae evtstrm crc32
CPU implementer	: 0x41
CPU architecture: 7
CPU variant	: 0x0
CPU part	: 0xd08
CPU revision	: 3
----
Architecture:        armv7l
Byte Order:          Little Endian
CPU(s):              4
On-line CPU(s) list: 0-3
Thread(s) per core:  1
Core(s) per socket:  4
Socket(s):           1
Vendor ID:           ARM
Model:               3
Model name:          Cortex-A72
Stepping:            r0p3
CPU max MHz:         1500.0000
CPU min MHz:         600.0000
BogoMIPS:            270.00
Flags:               half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 lpae evtstrm crc32

 *
 */
static std::string cpuInfo()
{
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0,0,0,0};
    memset(CPUBrandString, 0, sizeof(CPUBrandString));


    // 0x16 not working for amd as eax is 1 for call 0x0!
    //   http://www.felixcloutier.com/x86/CPUID.html
    //   https://www.microbe.cz/docs/CPUID.pdf
    __cpuid(0x00000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nStd = CPUInfo[0];
    memcpy(CPUBrandString, &CPUInfo[1], sizeof(CPUInfo[1]));
    memcpy(CPUBrandString+4, &CPUInfo[3], sizeof(CPUInfo[3]));
    memcpy(CPUBrandString+8, &CPUInfo[2], sizeof(CPUInfo[2]));
    std::cout << "std " << nStd << " id " << CPUBrandString << std::endl;

    CPUInfo[0] = 0;
    CPUInfo[1] = 0;
    CPUInfo[2] = 0;
    CPUInfo[3] = 0;
    __cpuid(0x00000001, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    std::cout << "eax 0x" << std::hex << CPUInfo[0] << std::endl;
    std::cout << "ebx 0x" << std::hex << CPUInfo[1] << std::endl;
    std::cout << "ecx 0x" << std::hex << CPUInfo[2] << std::endl;
    std::cout << "edx 0x" << std::hex << CPUInfo[3] << std::endl;

    CPUInfo[0] = 0;
    CPUInfo[1] = 0;
    CPUInfo[2] = 0;
    CPUInfo[3] = 0;
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    std::cout << "Brand " << CPUBrandString << std::endl;

    return std::string(CPUBrandString);
#else
    return std::string("2000M");	// find something working
#endif
}

void
ClkMonitor::decodeMaxClock(std::string &cpu)
{
    // works for intel cpu's which give a realistic rating
    //   amd's is rather virtual (3200 for 2000 real) see lscpu for something better ?
    bool num = false;
    unsigned int n = 0;
    unsigned int pos = cpu.length();
    char unit = '\0';
    for (std::string::iterator p = cpu.end(); p != cpu.begin(); --p) {
        char c = *p;
        if ((c >= '0' && c <= '9') || c == '.') {
            if (!num) {
                num = true;
            }
            ++n;
        }
        else if (!num && (c == 'M' || c == 'G')) {
            unit = c;
        }
        else if (num) {
            ++pos;
            std::string clk = cpu.substr(pos, n);
            sscanf(clk.c_str(), "%lf", &maxMHz);
            if (unit == 'G') {
                maxMHz *= 1000.0;
            }
            break;
        }
        --pos;
    }
}

float
ClkMonitor::readMaxFreq()
{
    // expect desktop cpus have the same max frequency across cores
    FileByLine stat;
    if (!stat.open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r")) {
        std::cerr << "Coud not read max freq" << std::endl;
        return 1.0f;
    }
    double fMaxkHz = 1.0;
    int cnt = fscanf(stat.getFile(), "%lf", &fMaxkHz);
    if (cnt == 1) {
        maxMHz = (float)(fMaxkHz / 1.0e3);
    }
    return maxMHz;
}

// compute a average clock from time_in_state per core
double
ClkMonitor::clock_avg(const std::string &cpu_dir, unsigned int clock_steps[], unsigned int clock_time[])
{
    std::string cpu_stat(cpu_dir);
    cpu_stat += "/cpufreq/stats/time_in_state";
    FileByLine stat;
    size_t len;
    if (!stat.open(cpu_stat, "r")) {
        // for x86 the time in state is missing use actual
        cpu_stat = cpu_dir;
        cpu_stat += "/cpufreq/scaling_cur_freq";
        FileByLine curFreq;
        if (!curFreq.open(cpu_stat, "r")) {
            //g_warning("monitors: Could not open .../cpufreq/stats/time_in_state and %s: %d, %s",
            //        cpu_stat.c_str(), errno, strerror(errno) );
            return 0.0; // do not complain as we will use leagacy afterwards
        }
        double act_freq = 0.0;
        const char *buf = curFreq.nextLine(&len);
        if (buf != nullptr) {
            unsigned int freq;
            if (sscanf(buf, "%u\n", &freq) == 1)
            {
                act_freq = (double)freq / 1.0e3;    // kHz -> MHz
            }
        }
        return act_freq;
    }
    guint index = 0;
    guint dt[CLOCKS];
    while (TRUE)  {
        const char *buf = stat.nextLine(&len);
        if (buf == nullptr) {
            break;
        }
        unsigned int freq;
        unsigned int time;
        if (sscanf(buf, "%u %u\n", &freq, &time) == 2)
        {
            if (clock_steps[index] == 0)    // initalisation
            {
                clock_steps[index] = freq;
                clock_time[index] = time;
            }
            else     // evaluation
            {        // are all steps are always present ??? hope so
                if (clock_steps[index] == freq)
                {
                    dt[index] = time - clock_time[index];
                    clock_time[index] = time;
                }
                else
                {
                    g_warning("monitors: The expected freq  %d (is %d) for %s no matching frequency",
                        freq, clock_steps[index], cpu_stat.c_str());
                }
            }
            ++index;
            if (index >= CLOCKS) {
                break;      // this is the limit we can handle...
            }
        }
    }
    guint sum_time = 0;
    for (guint i = 0; i < index; ++i)
    {
        sum_time += dt[i];
    }
    double avg_freq = 0.0;
    for (guint i = 0; i < index; ++i)
    {
        double share = (double)dt[i] / (double)sum_time;
        //std::cout << "dt " << dt[i] << " sum " << sum_time << " freq " << clock_steps[i] << std::endl;
        avg_freq += (double)clock_steps[i] * share;
    }
    avg_freq /= 1.0e3;                  // to get MHz from kHz
    avg_freq = min(avg_freq, maxMHz);   // on the first round we get unreasonable values. Due to time overflow ?
    //std::cout << "avg " << avg_freq << " max " << maxMHz  << std::endl;
    return avg_freq;
}

void
ClkMonitor::leagacyFreq()
{
// one shot, not avail for arm
    char buf[80];
    unsigned int test = 0;
    double cpu_clock;
    //stream will not work...
    FILE *stat = fopen("/proc/cpuinfo", "r");
    if (!stat) {
        g_warning("monitors: Could not open /proc/cpuinfo: %d, %s",
                errno, strerror(errno) );
        m_enabled = false;
    }
    while (fgets(buf, sizeof(buf), stat))  {  // !stat.eof()
        //stat.getline(buf, sizeof(buf));
        if (sscanf(buf, "processor	: %u\n", &test) == 1) {
            cpus = test;
        }
        else if (sscanf(buf, "cpu MHz		: %lf\n", &cpu_clock) == 1) {
            if (cpus < CLOCKS) {
                clkMHz[cpus] = cpu_clock;
                cpus++;
                if (maxMHz < cpu_clock) {
                    maxMHz = cpu_clock;
                }
            }
        }
    }
    fclose(stat);
}


gboolean
ClkMonitor::update(int refreshRate, glibtop * glibtop)
{
    if (maxMHz <= 1.0f) {
        readMaxFreq();
        if (maxMHz <= 1.0f) {   // if first try failed, try cpuid approach
            std::string cpu = cpuInfo();
            decodeMaxClock(cpu);
        }
    }
    cpus = 0;
    // try to resolve clock with time resolution
    DIR *dir;
    struct dirent *ent;
    const char *sdir = "/sys/devices/system/cpu/";
    double sumFreq{0.0};
    if ((dir = opendir(sdir)) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR
                && strncmp(ent->d_name, "cpu", 3) == 0) {
                char *endptr;
                char *num_ptr = ent->d_name + 3;
                unsigned int cpu = strtol(num_ptr, &endptr, 10);
                if (endptr != num_ptr
                    && cpu < CLOCKS) {
                    std::string cpuDir(sdir);
                    cpuDir += std::string(ent->d_name);
                    clkMHz[cpu] = clock_avg(cpuDir, m_clockStep[cpu], m_clockTime[cpu]);
                    sumFreq += clkMHz[cpu];
                    cpus = max(cpus, cpu + 1);
                }
            }
        }
        closedir(dir);
    }
    if (sumFreq == 0.0) {   // if advanced method failed use /proc
        leagacyFreq();
    }
    if (cpus > CLOCKS) {
        cpus = CLOCKS;      // limit to internal maximum, anyway displaying more than 16 lines is not useful
    }
    for (guint i = 0; i < cpus; ++i) {
        if (i >= m_Stats.size()) {
			auto buf = std::make_shared<Buffer<double>>(m_size);
            m_Stats.push_back(std::move(buf));
        }
        getValues(i)->set(clkMHz[i] / maxMHz);
    }
    return TRUE;
}

Gdk::RGBA *
ClkMonitor::getColor(unsigned int diagram)
{
    if (cpus > 1) {
        if (colors[diagram] == nullptr) {
            double r = m_foreground_color.get_red();
            double g = m_foreground_color.get_green();
            double b = m_foreground_color.get_blue();
            double dr = m_secondary_color.get_red() - r;
            double dg = m_secondary_color.get_green() - g;
            double db = m_secondary_color.get_blue() - b;
            double s = (double)diagram / (double)(cpus-1);
            colors[diagram] = new Gdk::RGBA();
            colors[diagram]->set_red(r+dr*s);
            colors[diagram]->set_green(g+dg*s);
            colors[diagram]->set_blue(b+db*s);
        }
        return colors[diagram];
    }
    return Monitor::getColor(diagram);
}


void
ClkMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{

    for (guint c = 0; c < MIN(cpus,4); ++c) {
        cr->move_to(1.0, (c+1)*10);
        auto temp = Glib::ustring::sprintf("%.1fMHz", clkMHz[c]);
        if (c == 0) {
            temp += Glib::ustring::sprintf("  Cpus %d", cpus);
        }
        cr->show_text(temp);
    }

//    cr->rectangle(60.5, 0.5, width-61, height-1);
//    cr->stroke();
//    double x = width - 1.5;
//    for (int i = m_size-1; i >= 0; --i)
//    {
//        cr->move_to(x, height-1);
//        float y = (gfloat)(height-1) - m_primaryStats.get(i)*(gfloat)(height-2);
//        cr->line_to(x, y);
//        x -= 1.0;
//        if (x <= 60.0) {
//            break;
//        }
//    }
//    cr->stroke();

}

void
ClkMonitor::load_settings(const Glib::KeyFile  * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_HW, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_HW_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(HW_PRIMARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_SEONDARY_HW_COLOR, m_secondary_color))
	m_secondary_color = Gdk::RGBA(HW_SECONDARY_DEFAULT_COLOR);

}

void
ClkMonitor::save_settings(Glib::KeyFile * settings)
{
    config_group_set_int(settings, m_name, CONFIG_DISPLAY_HW, m_enabled);
    config_group_set_color(settings, m_name, CONFIG_HW_COLOR, m_foreground_color);
    config_group_set_color(settings, m_name, CONFIG_SEONDARY_HW_COLOR, m_secondary_color);
}

std::string
ClkMonitor::getPrimMax()
{
    return formatScale(maxMHz/1.0e3, "GHz");
}

std::string
ClkMonitor::getSecMax()
{
    return std::string();
}
