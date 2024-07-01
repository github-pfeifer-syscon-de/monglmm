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
#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <cstdlib>
#include <errno.h>
#include <memory>

#include "config.h"
#include "TempMonitor.hpp"
#ifdef LMSENSORS
#include "LmSensor.hpp"
#include "LmSensors.hpp"
#endif
#ifdef RASPI
#include "RaspiSensor.hpp"
#include "RaspiSensors.hpp"
#endif


TempMonitor::TempMonitor(guint points)
: Monitor(points, "Temp")
, m_allSensors()
, m_actSensors()
, m_colors()
{
#ifdef LMSENSORS
    std::shared_ptr<LmSensors> lmSensors = std::make_shared<LmSensors>();
    m_allSensors.push_back(lmSensors);
#endif
#ifdef RASPI
    std::shared_ptr<RaspiSensors> raspiSensors = std::make_shared<RaspiSensors>();
    m_allSensors.push_back(raspiSensors);
#endif
    m_enabled = FALSE;          // not used at the moment
    // the number of m_colors is also the number of max displayable sensors
    // init defaults for colors
    m_colors.push_back(Gdk::RGBA("#ff0000"));
    m_colors.push_back(Gdk::RGBA("#0000ff"));
    m_colors.push_back(Gdk::RGBA("#00ff00"));
    m_colors.push_back(Gdk::RGBA("#ff00ff"));

}

TempMonitor::~TempMonitor()
{
    close();
}

unsigned long
TempMonitor::getTotal() {
    return 0ul;
}

Gtk::Box *
TempMonitor::create_config_page(MonglView *monglView)
{
    auto hw_box = create_config_page(
            _("Temperature"));
    return hw_box;
}

void
TempMonitor::sensor_changed(guint index, Gtk::ComboBox *combo)
{
    Glib::ustring name = combo->get_active_id();
    while (index >= m_actSensors.size()) {
        m_actSensors.push_back(nullptr);
    }
    if (!name.empty()) {
        for (auto sensors : m_allSensors) {
            auto sensor = sensors->getSensor(name);
            if (sensor) {
                m_actSensors[index] = sensor;
                break;
            }
        }
    }
    else {
        m_actSensors[index] = nullptr;        // clear reference
    }
}

void
TempMonitor::color_changed(guint index, Gtk::ColorButton *color_button)
{
    m_colors[index] = color_button->get_rgba();
}

void
TempMonitor::createEntry(Gtk::Box *box, guint index)
{
    auto color_button = Gtk::manage(new Gtk::ColorButton());
    color_button->set_rgba(m_colors[index]);
    color_button->signal_color_set().connect(
                    sigc::bind<guint, Gtk::ColorButton *>(
                    sigc::mem_fun(*this, &TempMonitor::color_changed),
                    index, color_button));

    auto hbox1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL, 0));
    hbox1->pack_start(*color_button, TRUE, TRUE, 0);
    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append("", "");  // allow disable
    for (auto sensors : m_allSensors) {
        for (auto mapEntry : sensors->getSensors()) {
            auto sensor = mapEntry.second;
            combo->append(sensor->getName(), sensor->getLabel());
        }
    }
    bool activated = false;
    if (index < m_actSensors.size()) {
        auto sensor = m_actSensors[index];
        if (sensor) {
            combo->set_active_id(sensor->getName());
            activated = true;
        }
    }
    if (!activated) {
        combo->set_active_id("");
    }
    combo->signal_changed().connect(
                    sigc::bind<guint, Gtk::ComboBox *>(
                    sigc::mem_fun(*this, &TempMonitor::sensor_changed),
                    index, combo));
    hbox1->pack_start(*combo, TRUE, TRUE, 0);
    char temp[64];
    snprintf(temp, sizeof(temp), "%s %d", CONFIG_DISPLAY_TEMP, index);
    add_widget2box(box, temp, hbox1, 0.0f);

}

Gdk::RGBA *
TempMonitor::getColor(unsigned int diagram)
{
    if (diagram < m_colors.size()) {
        return &m_colors[diagram];
    }
    return Monitor::getColor(diagram);
}

Gtk::Box *
TempMonitor::create_config_page(const char *enabledLabel)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));
    for (guint i = 0; i < m_colors.size(); ++i) {
        createEntry(box, i);
    }

    return box;
}


gboolean
TempMonitor::update(int refreshRate, glibtop * glibtop)
{
    int i = 0;
    for (auto s : m_actSensors) {
        if (s) {
            double value = s->read();
            double max = s->getMax();
            getValues(i)->set(value / max);
            ++i;
        }
    }
    return TRUE;
}

void
TempMonitor::close()
{
    m_actSensors.clear();
    m_allSensors.clear();
}


void
TempMonitor::updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height)
{
    int i = 0;
    for (auto s : m_actSensors) {
        if (s) {
    // might help to keep text adjusted further down the line
    //cairo_font_extents_t fe;
    //cairo_text_extents_t te;
    //char alphabet[] = "AbCdEfGhIjKlMnOpQrStUvWxYz";
    //char letter[2];
    //
    //cairo_font_extents (cr, &fe);
    //for (i=0; i < strlen(alphabet); i++) {
    //    *letter = '\0';
    //    strncat (letter, alphabet + i, 1);
    //cairo_text_extents (cr, letter, &te);
    //cairo_move_to (cr, i + 0.5 - te.x_bearing - te.width / 2,
    //        0.5 - fe.descent + fe.height / 2);
    //cairo_show_text (cr, letter);

            cr->move_to(0.5, (i+1)*10.0);
            std::ostringstream oss ;
            oss << formatScale(s->getMin(), s->getUnit().c_str(), 1000.0)
                << "< " << formatScale(s->read(), s->getUnit().c_str(), 1000.0)
                << " <" << formatScale(s->getMax(), s->getUnit().c_str(), 1000.0);
            cr->show_text(oss.str());
            ++i;
        }
    }
}

void
TempMonitor::load_settings(const Glib::KeyFile  * settings)
{
    config_setting_lookup_int(settings, m_name, CONFIG_DISPLAY_TEMP, &m_enabled);
    char confName[64];

    for (guint i = 0; i < m_colors.size(); ++i) {
        snprintf(confName, sizeof(confName), "%s%d", CONFIG_TEMP_COLOR, i);
        config_setting_lookup_color(settings, m_name, confName, m_colors[i]);

        snprintf(confName, sizeof(confName), "%s%d", CONFIG_TEMP_SENSOR, i);
        Glib::ustring sensorName;
        std::shared_ptr<Sensor> sensor;
        if (!config_setting_lookup_string(settings, m_name, confName, sensorName)) {
            if (m_allSensors.size() > 0) {
                std::shared_ptr<Sensors> sensores = m_allSensors[0];
                if (i == 0)
                    sensor = sensores->getPrimDefault();
                else if (i == 1)
                    sensor = sensores->getSecDefault();
            }
        }
        else {
            for (auto s : m_allSensors) {
                sensor = s->getSensor(sensorName.c_str());
                if (sensor) {
                    break;
                }
            }
        }
        if (sensor) {
            m_actSensors.push_back(sensor);
        }
    }
}

void
TempMonitor::save_settings(Glib::KeyFile * settings)
{
    config_group_set_int(settings, m_name, CONFIG_DISPLAY_TEMP, m_enabled);

    char confName[64];
    for (guint i = 0; i < m_colors.size(); ++i) {
        snprintf(confName, sizeof(confName), "%s%d", CONFIG_TEMP_COLOR, i);
        config_group_set_color(settings, m_name, confName, m_colors[i]);

        Glib::ustring name;
        if (i < m_actSensors.size()) {
            auto s = m_actSensors[i];
            if (s) {
                name = s->getName();
            }
        }
        snprintf(confName, sizeof(confName), "%s%d", CONFIG_TEMP_SENSOR, i);
        config_group_set_string(settings, m_name, confName, name);
    }
}

std::string
TempMonitor::getPrimMax()
{
    std::string str;
    if (m_actSensors.size() > 0) {
        std::shared_ptr<Sensor> primSensor = m_actSensors[0];
        if (primSensor) {
            str = Monitor::formatScale(primSensor->getMax(), primSensor->getUnit().c_str(), 1000.0);
        }
    }
    return str;
}

std::string
TempMonitor::getSecMax()
{
    std::string str;
    if (m_actSensors.size() > 1) {
        std::shared_ptr<Sensor> secSensor = m_actSensors[1];
        if (secSensor) {
            str = Monitor::formatScale(secSensor->getMax(), secSensor->getUnit().c_str(), 1000.0);
        }
    }
    return str;
}
