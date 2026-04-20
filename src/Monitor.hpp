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

#pragma once

#include <glibmm.h>
#include <string>
#include <memory>

#include "libgtop_helper.h"
#include "Geom2.hpp"
#include "Page.hpp"
#include "Buffer.hpp"

class MonglView;

struct FormatLimit
{
    constexpr FormatLimit(uint64_t size)
    : LIMIT_T{static_cast<double>(size*size*size*size)}
    , LIMIT_G{static_cast<double>(size*size*size)}
    , LIMIT_M{static_cast<double>(size*size)}
    , LIMIT_k{static_cast<double>(size)}
    , LIMIT_m{(1.0/LIMIT_k)}
    , LIMIT_u{(1.0/LIMIT_M)}
    , LIMIT_n{(1.0/LIMIT_G)}
    {
    }
    const double LIMIT_T;
    const double LIMIT_G;
    const double LIMIT_M;
    const double LIMIT_k;
    const double LIMIT_m;
    const double LIMIT_u;
    const double LIMIT_n;
};

class Monitor
: public Page {
public:
    Monitor(guint points, const char *_name);
    virtual ~Monitor();

    virtual gboolean update(int refreshRate, glibtop * glibtop) = 0;

    virtual void load_settings(const Glib::KeyFile * setting) = 0;

    virtual void save_settings(Glib::KeyFile * setting)  = 0;

    virtual Gtk::Box* create_config_page(MonglView *monglView)  = 0;

    virtual void roll();
    virtual void reinit();

    const char *m_name;

    Glib::ustring getDisplayName();

    unsigned int getNumDiagram() const;
    std::shared_ptr<Buffer<double>> getValues(unsigned int diagram);
    virtual Gdk::RGBA *getColor(unsigned int diagram);
    virtual unsigned long getTotal() = 0;
    virtual std::string getPrimMax() = 0;
    virtual std::string getSecMax() = 0;
    static std::string formatScale(double value, const char *suffix, uint64_t scale = 1024);
    static void add_widget2box(Gtk::Box *dest, const char *lbl, Gtk::Widget *toAdd, gfloat y_scale);
    virtual void close();
    virtual guint defaultValues();  // number of values used by default

    guint getSize() {
        return m_size;
    }
    void primary_color_changed(Gtk::ColorButton *primary_color);
    void secondary_color_changed(Gtk::ColorButton *secondary_color);
    void ternary_color_changed(Gtk::ColorButton *ternary_color);
protected:
    Gtk::Box* create_default_config_page(const char *enabledLabel,
                                        const char *primaryColorLabel,
                                        const char *secondaryColorLabel,
                                        const char *ternaryColorLabel = nullptr);
    gboolean m_enabled;
    guint m_size;       // number of values
    std::vector<std::shared_ptr<Buffer<double>>> m_Stats;

    Glib::ustring m_device;
    Glib::ustring m_used_device;

    Gdk::RGBA m_foreground_color;
    Gdk::RGBA m_secondary_color;
    Gdk::RGBA m_ternary_color;

    void toggle_changed(Gtk::ToggleButton *enabled);

    static constexpr FormatLimit formatLimitIec{1024ul};
    static constexpr FormatLimit formatLimitSi{1000ul};
private:

};



void  config_setting_lookup_int(const Glib::KeyFile * settings, const char *grp, const char *name, int *val);

gboolean config_setting_lookup_string(const Glib::KeyFile * settings, const char *grp, const char *name,  Glib::ustring &tmp);

gboolean config_setting_lookup_color(const Glib::KeyFile * settings, const char *grp, const char *name,  Gdk::RGBA &color);

void config_group_set_int(Glib::KeyFile *  settings, const char *grp, const char *name, int val) ;

void config_group_set_string(Glib::KeyFile * settings, const char *grp, const char *name, const Glib::ustring &tmp);

void config_group_set_color(Glib::KeyFile * settings, const char *grp, const char *name, const Gdk::RGBA &color);

gboolean config_setting_lookup_boolean(const Glib::KeyFile * settings, const char *grp, const char *name, bool useIfUndef);

gboolean has_setting(const Glib::KeyFile * settings, const char *grp, const char *name);

