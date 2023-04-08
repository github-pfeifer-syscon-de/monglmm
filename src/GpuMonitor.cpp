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

#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <gtkmm.h>
#include <errno.h>
#include <cmath>
#include <cairo.h>
#include <glib/gi18n.h>
#include <glibmm.h>
#include <string.h>
#include <vector>
#include <GenericGlmCompat.hpp>

#include "HistMonitor.hpp"
#include "G15Worker.hpp"
#include "GpuMonitor.hpp"
#include "GpuCounter.hpp"

#include "GpuExtAmdCounter.hpp"
#ifndef USE_GLES
#include "GpuGLquery.hpp"
#endif
#undef DEBUG
//#define DEBUG 1


GpuMonitor::GpuMonitor(guint points, Gtk::GLArea *glArea)
: HistMonitor(points, "GPU")
, m_counterPrim()
, m_counterSec()
, m_glArea{glArea}
{
    m_enabled = false;
}


GpuMonitor::~GpuMonitor()
{
}

void GpuMonitor::close()
{
	// require early destruction as gl-context is required + avoid message "end query..."
	m_counterPrim.reset();
	m_counterSec.reset();
    GpuExtAmdCounters::reset();
    #ifndef USE_GLES
    GpuGLqueries::reset();
    #endif
	Monitor::close();
}

void
GpuMonitor::reinit()
{
    previous_time = 0l;
}

Gtk::Box *
GpuMonitor::create_config_page()
{
    auto gpu_box = create_config_page(
            _("Display GPU usage"));

    return gpu_box;
}

void
GpuMonitor::counter_changed(int index, Gtk::ComboBox* combo)
{
    auto selected = combo->get_active_id();
    auto counter = find_by_name(selected);
    if (index == 0) {
        m_counterPrim = counter;
		m_primaryHist.reset();	// as previous values are misleading
		maxPrim = 0l;
    }
    else {
        m_counterSec = counter;
		m_secondaryHist.reset();
		maxSec = 0l;
    }
}

void
GpuMonitor::color_changed(int index, Gtk::ColorButton* colorButton)
{
    auto color = colorButton->get_rgba();
    if (index == 0) {
        m_foreground_color = color;
    }
    else {
        m_secondary_color = color;
    }
}

void
GpuMonitor::createEntry(Gtk::Box *box,
                        int index,
                        const std::string& counter_selected,
                        Gdk::RGBA color_selected)
{
    auto color_button = Gtk::manage(new Gtk::ColorButton());
    color_button->set_rgba(color_selected);
    color_button->signal_color_set().connect(
                    sigc::bind<int, Gtk::ColorButton *>(
                    sigc::mem_fun(*this, &GpuMonitor::color_changed),
                    index, color_button));

    auto hbox1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0));
    hbox1->pack_start(*color_button, TRUE, TRUE, 0);
    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append("", "");  // allow disable
    auto allNames = list();
    for (auto name : allNames) {
        combo->append(name, name);
    }
    combo->set_active_id(counter_selected);
    combo->signal_changed().connect(
                    sigc::bind<int, Gtk::ComboBox *>(
                    sigc::mem_fun(*this, &GpuMonitor::counter_changed),
                    index, combo));
    hbox1->pack_start(*combo, TRUE, TRUE, 0);
    char temp[64];
    snprintf(temp, sizeof(temp), "%s %s", CONFIG_DISPLAY_GPU, index == 0 ? _("Primary") : _("Secondary"));
    add_widget2box(box, temp, hbox1, 0.0f);

}


Gtk::Box *
GpuMonitor::create_config_page(const char *enabledLabel)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));
    createEntry(box, 0, m_counterPrim ? m_counterPrim->get_name() : "", m_foreground_color);
    createEntry(box, 1, m_counterSec ? m_counterSec->get_name() : "", m_secondary_color);
	#ifdef DEBUG
	std::cout << "GpuMonitor::create_config_page" << std::endl;
	#endif
    return box;
}


gboolean
GpuMonitor::update(int refreshRate, glibtop * glibtop)
{
	if (m_counterPrim || m_counterSec) {
		auto amdCounters = GpuExtAmdCounters::create();
		amdCounters->read();

		gint64 actual_time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
		guint64 dt = refreshRate * 1e6l;
		if (previous_time != 0) {
			dt = (actual_time - previous_time);
		}
		if (m_counterPrim) {
			m_counterPrim->update(m_primaryHist, dt, refreshRate);
		}
		if (m_counterSec) {
			m_counterSec->update(m_secondaryHist, dt, refreshRate);
		}
		maxPrim = m_primaryHist.getMax();
		#ifdef DEBUG
		std::cout << "Max "<<  maxPrim << std::endl;
		#endif
		maxSec = m_secondaryHist.getMax();
		for (guint i = 0; i < m_size; i++) {
			getValues(0)->set(i, m_primaryHist.get(i) / (double)maxPrim);
			getValues(1)->set(i, m_secondaryHist.get(i) / (double)maxSec);
		}
		getValues(0)->refreshSum();
		getValues(1)->refreshSum();
		previous_time = actual_time;
	}
    return true;
}


unsigned long
GpuMonitor::getTotal()
{
    guint64 fmax = MAX(m_primaryHist.getMax(), m_secondaryHist.getMax());
    return fmax;
}

void
GpuMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
//    guint64 maxPrim = m_primaryHist.getMax();
//    float fPrim = maxPrim;
//    guint64 maxSec = m_secondaryHist.getMax();
//    float fSec = maxSec;
//
//    cr->move_to(1.0, 10.0);
//    cr->show_text("GPU");
//    cr->move_to(1.0, 20.0);
//    cr->show_text("P");
//    cr->move_to(10.0, 20.0);
//    std::string total = formatScale(fPrim, "");
//    cr->show_text(total);
//    cr->move_to(1.0, 30.0);
//    cr->show_text("S");
//    cr->move_to(10.0, 30.0);
//    total = formatScale(fSec, "");
//    cr->show_text(total);
//
//
    cr->rectangle(60.5, 0.5, width-61, height-1);
    cr->stroke();
//    double x = width - 1.5;
//    for (int i = m_size-1; i >= 0; --i)
//    {
//        cr->move_to(x, height-1);
//        float y = (float)(height-1) - (m_secondaryHist.get(i)/fSec)*(gfloat)(height-2);
//        cr->line_to(x, y);
//        x -= 1.0;
//        if (x <= 60.0) {
//            break;
//        }
//    }
//    cr->stroke();

}

std::list<std::string>
GpuMonitor::list()
{
    std::list<std::string> names;
    auto amdCounters = GpuExtAmdCounters::create();
    auto amdNames = amdCounters->list();
    for (auto amdName : amdNames) {
        names.push_back(amdName);
    }
#ifndef USE_GLES
    auto queryCounters = GpuGLqueries::create();
    auto queryNames = queryCounters->list();
    for (auto queryName : queryNames) {
        names.push_back(queryName);
    }
#endif
    return names;
}

std::shared_ptr<GpuCounter>
GpuMonitor::find_by_name(const std::string& name)
{
	// we require the correct gl_context for setup counters/query
	m_glArea->make_current();
    if (name != "") {
        auto amdCounters = GpuExtAmdCounters::create();
        auto amdCounter = amdCounters->get(name);
        if (amdCounter) {
            return amdCounter;
        }
    #ifndef USE_GLES
        auto queryCounters = GpuGLqueries::create();
        auto queryCounter = queryCounters->get(name);
        if (queryCounter) {
            return queryCounter;
        }
    #endif
    }
    return nullptr;
}

void
GpuMonitor::load_settings(const Glib::KeyFile * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_GPU, &m_enabled);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_GPU_COLOR, m_foreground_color))
        m_foreground_color = Gdk::RGBA(GPU_PRIMARY_DEFAULT_COLOR);
    if (!config_setting_lookup_color(settings, m_name, CONFIG_GPU_SECONDARY_COLOR, m_secondary_color))
        m_secondary_color = Gdk::RGBA(GPU_SECONDARY_DEFAULT_COLOR);
    Glib::ustring primaryName;
    if (config_setting_lookup_string(settings, m_name, CONFIG_GPU_CONFIG_PRIMARY_NAME, primaryName)) {
        m_counterPrim = find_by_name(primaryName);
    }
    Glib::ustring secondaryName;
    if (config_setting_lookup_string(settings, m_name, CONFIG_GPU_CONFIG_SECONDARY_NAME, secondaryName)) {
        m_counterSec = find_by_name(secondaryName);
    }


}

void
GpuMonitor::save_settings(Glib::KeyFile * setting)
{
    config_group_set_int(setting, m_name, CONFIG_DISPLAY_GPU, m_enabled);
    config_group_set_color(setting, m_name, CONFIG_GPU_COLOR, m_foreground_color);
    config_group_set_color(setting, m_name, CONFIG_GPU_SECONDARY_COLOR, m_secondary_color);
    config_group_set_string(setting, m_name, CONFIG_GPU_CONFIG_PRIMARY_NAME, m_counterPrim ? m_counterPrim->get_name() : "");
    config_group_set_string(setting, m_name, CONFIG_GPU_CONFIG_SECONDARY_NAME, m_counterSec ? m_counterSec->get_name() : "");
}

std::string
GpuMonitor::getPrimMax()
{
    if (m_counterPrim) {
        auto prim = formatScale(maxPrim, m_counterPrim->get_unit().c_str());
        return prim;
    }
    return "";
}

std::string
GpuMonitor::getSecMax()
{
    if (m_counterSec) {
        auto sec = formatScale(maxSec, m_counterSec->get_unit().c_str());
        return sec;
    }
	return "";
}
