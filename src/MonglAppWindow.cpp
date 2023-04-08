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
#include <thread>
#include <future>
#include <GenericGlmCompat.hpp>
#include <gtkmm.h>

#include "G15Worker.hpp"
#include "Monitor.hpp"
#include "MonglView.hpp"
#include "MonglAppWindow.hpp"
#include "MonglApp.hpp"

MonglAppWindow::MonglAppWindow(Gtk::Application* application)
: Gtk::ApplicationWindow()
, m_monglView{nullptr}
, m_application{application}
{
    Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_resource(
            application->get_resource_base_path() + "/monglmm.png");
    set_icon(pix);
    m_monglView = new MonglView(application);
    auto naviGlArea = Gtk::manage(new NaviGlArea(m_monglView));
	#ifdef USE_GLES
    //naviGlArea->set_required_version (3, 0);
	naviGlArea->set_use_es(true);
	#endif
    add(*naviGlArea);

    set_default_size(480, 640);
    show_all_children();
}


MonglAppWindow::~MonglAppWindow()
{
    //std::cout << "MonglAppWindow::~MonglAppWindow " << std::endl;
}

void
MonglAppWindow::on_action_preferences()
{
    Gtk::Dialog *dlg = m_monglView->monitors_config();
    dlg->set_transient_for(*this);
    dlg->run();
    m_monglView->save_config();
    dlg->hide();
    delete dlg;
}

void
MonglAppWindow::on_action_about()
{
    auto refBuilder = Gtk::Builder::create();
    try {
        refBuilder->add_from_resource(
            m_application->get_resource_base_path() + "/abt-dlg.ui");
        auto object = refBuilder->get_object("abt-dlg");
        auto abtdlg = Glib::RefPtr<Gtk::AboutDialog>::cast_dynamic(object);
        if (abtdlg) {
            Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_resource(
                    m_application->get_resource_base_path() +"/monglmm.png");
            abtdlg->set_logo(pix);
            abtdlg->set_transient_for(*this);
            abtdlg->run();
            abtdlg->hide();
        } else
            std::cerr << "MonglAppWindow::on_action_about(): No \"abt-dlg\" object in abt-dlg.ui"
                << std::endl;
    } catch (const Glib::Error& ex) {
        std::cerr << "MonglAppWindow::on_action_about(): " << ex.what() << std::endl;
    }

}
