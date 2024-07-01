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

#include <gtkmm.h>
#include <glibmm.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "Monitor.hpp"
#include "StringUtils.hpp"

Monitor::Monitor(guint points, const char *_name)
: m_name{_name}
, m_enabled{true}
, m_size{points}
, m_device()
, m_used_device()
{
}

Monitor::~Monitor()
{
}

void
Monitor::roll() {
    for (auto p : m_Stats) {
        p->roll();
    }
}

std::shared_ptr<Buffer<double>>
Monitor::getValues(unsigned int diagram) {
    while (m_Stats.size() <= diagram) {
		auto buf = std::make_shared<Buffer<double>>(m_size);
        m_Stats.push_back(buf);  // make this dynamic
    }
    return m_Stats[diagram];
}

guint
Monitor::defaultValues()
{
	return 2;	// number of diagrams used by default important if get extended e.g. mem,cpu
}

Gdk::RGBA *
Monitor::getColor(unsigned int diagram) {
    switch (diagram) {
    case 0 :
        return &m_foreground_color;
    case 1:
        return &m_secondary_color;
    case 2:
		if (defaultValues() > 2) {
			return &m_ternary_color;
		}
    }
    return &m_secondary_color;
}


void
config_setting_lookup_int(const Glib::KeyFile * settings, const char *grp, const char *name, int *val)
{
    if (has_setting(settings, grp, name))
        *val = settings->get_integer(grp, name);
}

gboolean
config_setting_lookup_string(const Glib::KeyFile * settings, const char *grp, const char *name,  Glib::ustring &tmp)
{
    if (has_setting(settings, grp, name)) {
        tmp = settings->get_string(grp, name);
        return true;
    }
    return false;
}

gboolean
config_setting_lookup_color(const Glib::KeyFile * settings, const char *grp, const char *name,  Gdk::RGBA &color)
{
    if (has_setting(settings, grp, name)) {
        Glib::ustring str = settings->get_string(grp, name);
        color = Gdk::RGBA(str);
        return true;
    }
    return false;
}

gboolean
config_setting_lookup_boolean(const Glib::KeyFile * settings, const char *grp, const char *name, bool useIfUndef)
{
    if (has_setting(settings, grp, name)) {
        useIfUndef = settings->get_boolean(grp, name);
    }
    return useIfUndef;
}

gboolean
has_setting(const Glib::KeyFile * settings, const char *grp, const char *name)
{
    if (settings != nullptr && settings->has_group(grp)) {
        if (settings->has_key(grp, name)) {
            return true;
        }
    }
    return false;
}

void
config_group_set_int(Glib::KeyFile * settings, const char *grp, const char *name, int val)
{
    settings->set_integer(grp, name, val);
}

void
config_group_set_string(Glib::KeyFile * settings, const char *grp, const char *name, const Glib::ustring &tmp)
{
    settings->set_string(grp, name, tmp);
}


void
config_group_set_color(Glib::KeyFile * settings, const char *grp, const char *name, const Gdk::RGBA &color)
{
    gchar *colorS = g_strdup_printf("#%04x%04x%04x",
                           color.get_red_u(), color.get_green_u(), color.get_blue_u());
    settings->set_string(grp, name, colorS);
    g_free(colorS);
}

void
Monitor::reinit()
{
}

void
Monitor::primary_color_changed(Gtk::ColorButton *primary_color)
{
    m_foreground_color = primary_color->get_rgba();
}

void
Monitor::secondary_color_changed(Gtk::ColorButton *secondary_color)
{
    m_secondary_color = secondary_color->get_rgba();
}

void
Monitor::ternary_color_changed(Gtk::ColorButton *ternary_color)
{
    m_ternary_color = ternary_color->get_rgba();
}

void
Monitor::toggle_changed(Gtk::ToggleButton *enabled)
{

    m_enabled = enabled->get_active();
    if (!m_enabled) {
        reinit();
    }
}

void
Monitor::add_widget2box(Gtk::Box *dest, const char *lbl, Gtk::Widget *toAdd, gfloat y_scale)
{
    auto _frame = Gtk::manage(new Gtk::Frame(lbl));
    _frame->set_shadow_type(Gtk::ShadowType::SHADOW_NONE);
    auto _align = Gtk::manage(new Gtk::Alignment(0, 0, 0, y_scale));
    //gtk_alignment_set_padding(GTK_ALIGNMENT(_align), 4, 4, 4, 4);
    _frame->add(*_align);
    _align->add(*toAdd);
    dest->pack_start(*_frame, TRUE, TRUE, 0);
}

Gtk::Box *
Monitor::create_default_config_page(
                                    const char *enabledLabel,
                                    const char *primaryColorLabel,
                                    const char *secondaryColorLabel,
                                    const char *ternaryColorLabel)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));

    // we dont care about enable-state at the moment
    //auto enabled_button = Gtk::manage(new Gtk::CheckButton());
    //enabled_button->set_active(m_enabled);
    //add_widget2box(box, enabledLabel, enabled_button, 0.0f);
    //enabled_button->signal_toggled().connect(
    //                                         sigc::bind<Gtk::CheckButton *>(
    //                                         sigc::mem_fun(*this, &Monitor::toggle_changed),
    //                                         enabled_button));

    auto primary_color_button = Gtk::manage(new Gtk::ColorButton());
    primary_color_button->set_rgba(m_foreground_color);
    add_widget2box(box, primaryColorLabel, primary_color_button, 0.0f);
    primary_color_button->signal_color_set().connect(sigc::bind<Gtk::ColorButton *>(
                                                     sigc::mem_fun(*this, &Monitor::primary_color_changed),
                                                     primary_color_button));

    if (secondaryColorLabel != nullptr) {
        auto secondary_color_button = Gtk::manage(new Gtk::ColorButton());
        secondary_color_button->set_rgba(m_secondary_color);
        add_widget2box(box, secondaryColorLabel, secondary_color_button, 0.0f);
        secondary_color_button->signal_color_set().connect(sigc::bind<Gtk::ColorButton *>(
                                                           sigc::mem_fun(*this, &Monitor::secondary_color_changed),
                                                           secondary_color_button));
    }
    if (ternaryColorLabel != nullptr) {
        auto ternary_color_button = Gtk::manage(new Gtk::ColorButton());
        ternary_color_button->set_rgba(m_ternary_color);
        add_widget2box(box, ternaryColorLabel, ternary_color_button, 0.0f);
        ternary_color_button->signal_color_set().connect(sigc::bind<Gtk::ColorButton *>(
                                                           sigc::mem_fun(*this, &Monitor::ternary_color_changed),
                                                           ternary_color_button));
    }
    return box;
}


Glib::ustring
Monitor::getDisplayName()
{
    Glib::ustring sname(StringUtils::lower(m_name, 1));

    return sname;
}

// move to StringUtils unify with Picnic
std::string
Monitor::formatScale(double value, const char *suffix, double scaleFactor)
{
    std::string scale;
    double mscale = scaleFactor * scaleFactor;
    double gscale = mscale * scaleFactor;
    if (value >= gscale) {
        value /= gscale;
        scale = "G";
    }
    else if (value >= mscale) {
        value /= mscale;
        scale = "M";
    }
    else if (value >= scaleFactor) {
        value /= scaleFactor;
        scale = "k";
    }
    else if (value < 10.0 / scaleFactor) {
        value *= scaleFactor;
        scale = "m";
    }
    else if (value < 10.0 / mscale) {
        value *= mscale;
        scale = "µ";
    }
    else if (value < 10.0 / gscale) {
        value *= gscale;
        scale = "n";
    }
    guint places = 0;
    if (value < 1.0) {
        places = 3;
    }
    else if (value < 10.0) {
        places = 2;
    }
    else if (value < 100.0) {
        places = 1;
    }
    std::ostringstream oss1;
    oss1.precision(places);
    oss1 << std::fixed << value;
    oss1 << scale;
    oss1 << suffix;
    std::string max = oss1.str();
    return max;
}

void Monitor::close()
{
}


unsigned int
Monitor::getNumDiagram() const
{
    return m_Stats.size();
}