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

#include <gtkmm.h>
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <Log.hpp>
#include <Geom2.hpp>
#include <Matrix.hpp>
#include <NaviGlArea.hpp>
#include <Scene.hpp>
#include <Font2.hpp>

#include "libgtop_helper.h"
#include "GraphShaderContext.hpp"
#include "Processes.hpp"
#include "Monitor.hpp"
#include "Filesyses.hpp"
#include "DiskInfos.hpp"
#include "TextContext.hpp"
#include "DiagramMonitor.hpp"
#include "config.h"
#include "NetInfo.hpp"
#ifdef LIBG15
#include "G15Worker.hpp"
#else
class G15Worker;
#endif

class MonglView : public Scene {
public:
    MonglView(Gtk::Application* application);
    virtual ~MonglView();


    gboolean init_shaders(Glib::Error &error) override;
    void init(Gtk::GLArea *glArea) override;
    void draw(Gtk::GLArea *glArea, Matrix &proj, Matrix &view) override;
    void unrealize () override;
    Position getIntialPosition() override;
    Rotational getInitalAngleDegree() override;

    void read_config();
    void save_config();

    Gtk::Dialog* monitors_config ();
    psc::gl::aptrGeom2 on_click_select(GdkEventButton* event, float mx, float my) override;
    bool selectionChanged(const psc::gl::aptrGeom2& prev_selected, const psc::gl::aptrGeom2& selected) override;
    bool on_click(GdkEventButton* event, float mx, float my) override;
    void create_popup();
    void on_process_properties();
    void restore();
    void close();
    void showMessage(const Glib::ustring& msg, Gtk::MessageType msgType = Gtk::MessageType::MESSAGE_INFO);
    Glib::KeyFile* getConfig();
    gint getUpdateInterval();
    static constexpr auto CONFIG_SHOW_NET_CONNECT = "showNetConnections";
    static constexpr auto CONFIG_GRP_MAIN = "Main";
    void net_connections_show_changed(Gtk::CheckButton* showNetConn);
protected:

private:
    GraphShaderContext *m_graph_shaderContext;
    TextContext *m_textContext;
    psc::gl::ptrFont2 m_font2;

    sigc::connection    m_timer;               /* Timer for regular updates     */

    gint m_updateInterval;

    glibtop *m_glibtop;                        /* portable way to get infos (needs .configure --with-glibtop) */
    std::vector<std::shared_ptr<DiagramMonitor>> m_diagrams;

    Glib::KeyFile* m_config;

    G15Worker *worker;
    std::thread *m_WorkerThread;
    Glib::Dispatcher m_Dispatcher;  // used for thread notification

    gboolean monitors_update();
    void update_timer();
    void update_interval_changed(Gtk::SpinButton* refresh_spin);
    void text_color_changed(Gtk::ColorButton* text_color);
    void process_type_changed(Gtk::ComboBoxText* process_type);
    void background_color_changed(Gtk::ColorButton* background_color);
    void on_notification_from_worker_thread();
    void drawContent();
    Processes m_processes;
    NaviGlArea *naviGlArea;
    Gtk::Menu m_popupMenu;
    std::shared_ptr<Filesyses> m_filesyses;
    std::shared_ptr<DiskInfos> m_diskInfos;
    Gtk::Application* m_application;

    Gdk::RGBA m_text_color;
    Gdk::RGBA m_background_color;
    std::shared_ptr<Monitor> m_temp;
    std::shared_ptr<NetInfo> m_netInfo;
    std::shared_ptr<psc::log::Log> m_log;
    static constexpr auto CONFIG_LOGLEVEL = "logLevel";
    static constexpr auto MIN_UPDATE_PERIOD = 1;              /* Seconds (minimum)    */
    static constexpr auto MAX_UPDATE_PERIOD = 60;              /* Seconds (maximum)    */
    static constexpr auto CONFIG_UPDATE_INTERVAL = "UpdateInterval";
    static constexpr auto CONFIG_TEXT_COLOR = "TextColor";
    static constexpr auto CONFIG_BACKGOUNDCOLOR = "BackgroundColor";
    static constexpr auto CONFIG_PROCESSTYPE = "processType";
    static constexpr auto TEXT_DEFAULT_COLOR = "#AAAAAA";
    static constexpr auto BACKGROUND_DEFAULT_COLOR = "#0F0F1F";
    static constexpr auto DIAGRAM_GAP = 0.2f;
    static constexpr auto n_values = 100u;
};

