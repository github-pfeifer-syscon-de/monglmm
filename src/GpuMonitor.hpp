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

#include <string>
#include <memory>
#include <map>
#include <GenericGlmCompat.hpp>

class GpuCounter;

class GpuMonitor : public HistMonitor
{
public:
    GpuMonitor(guint points, Gtk::GLArea *glArea);
    virtual ~GpuMonitor();

    virtual void close() override;
    gboolean update(int refreshRate, glibtop * glibtop) override;
    void updateG15(Cairo::RefPtr<Cairo::Context> cr, guint width, guint height) override;

    void load_settings(const Glib::KeyFile * setting) override;

    void save_settings(Glib::KeyFile * setting) override;

    Gtk::Box* create_config_page(MonglView *monglView) override;

    void reinit() override;
    unsigned long getTotal() override;
    std::string getPrimMax() override;
    std::string getSecMax() override;
protected:
    Gtk::Box * create_config_page(const char *enabledLabel);
    void createEntry(Gtk::Box *box, int index, const std::string& counter_selected, Gdk::RGBA color_selected);
    std::shared_ptr<GpuCounter> find_by_name(const std::string& name);
    std::list<std::string> list();
    void counter_changed(int index, Gtk::ComboBox* combo);
    void color_changed(int index, Gtk::ColorButton* colorButton);
private:
    gint64 previous_time{0l};
    guint64 maxPrim{0l};
    guint64 maxSec{0l};
    std::shared_ptr<GpuCounter> m_counterPrim;
    std::shared_ptr<GpuCounter> m_counterSec;
    Gtk::GLArea *m_glArea;
    static constexpr auto GPU_PRIMARY_DEFAULT_COLOR = "#00FF00";
    static constexpr auto GPU_SECONDARY_DEFAULT_COLOR = "#FF0000";
    static constexpr auto CONFIG_DISPLAY_GPU = "DisplayGPU";
    static constexpr auto CONFIG_GPU_COLOR = "GPUprimaryColor";
    static constexpr auto CONFIG_GPU_SECONDARY_COLOR = "GPUsecondaryColor";
    static constexpr auto CONFIG_GPU_CONFIG_PRIMARY_NAME = "GPUprimaryName";
    static constexpr auto CONFIG_GPU_CONFIG_SECONDARY_NAME = "GPUsecondaryName";
};
