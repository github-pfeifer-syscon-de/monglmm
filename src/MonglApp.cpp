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
#include <iostream>
#include <exception>
#include <thread>
#include <future>

#include "MonglAppWindow.hpp"
#include "MonglApp.hpp"

MonglApp::MonglApp(int argc, char **argv)
: Gtk::Application{argc, argv, "de.pfeifer_syscon.monglmm"}
, m_monglAppWindow{nullptr}
{

    //std::cout << "MonglApp::MonglApp" << std::endl;
}

MonglApp::MonglApp(const MonglApp& orig)
{
}

MonglApp::~MonglApp()
{
    //std::cout << "MonglApp::~MonglApp" << std::endl;
}

void
MonglApp::on_activate()
{
    //std::cout << "MonglApp::on_activate" << std::endl;
    add_window(*m_monglAppWindow);
    m_monglAppWindow->show();
}


void
MonglApp::on_action_quit()
{
    //std::cout << "MonglApp::on_action_quit" << std::endl;
    if (m_monglAppWindow) {
       m_monglAppWindow->hide();
    }

    // Not really necessary, when Gtk::Widget::hide() is called, unless
    // Gio::Application::hold() has been called without a corresponding call
    // to Gio::Application::release().
    std::cout << "quit " << std::endl;
    quit();
}

void
MonglApp::on_shutdown()
{
    // this is important as we crash otherwise on exit
    //std::cout << "MonglApp::on_shutdown" << std::endl;
    if (m_monglAppWindow) {
        delete m_monglAppWindow;
        m_monglAppWindow = nullptr;
    }
}

void
MonglApp::on_startup()
{
    // Call the base class's implementation.
    //std::cout << "MonglApp::on_startup" << std::endl;
    Gtk::Application::on_startup();
    //std::cout << "MonglApp::on_startup 2" << std::endl;
    m_monglAppWindow = new MonglAppWindow(this);
    signal_shutdown().connect(sigc::mem_fun(*this, &MonglApp::on_shutdown));

    // Add actions and keyboard accelerators for the application menu.
    add_action("preferences", sigc::mem_fun(*m_monglAppWindow, &MonglAppWindow::on_action_preferences));
    add_action("viewLog", sigc::mem_fun(*m_monglAppWindow, &MonglAppWindow::on_action_viewLog));
    add_action("about", sigc::mem_fun(*m_monglAppWindow, &MonglAppWindow::on_action_about));
    add_action("quit", sigc::mem_fun(*this, &MonglApp::on_action_quit));
    set_accel_for_action("app.quit", "<Ctrl>Q");

    auto refBuilder = Gtk::Builder::create();
    try {
        refBuilder->add_from_resource(get_resource_base_path() + "/app-menu.ui");
        auto object = refBuilder->get_object("appmenu");
        auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
        if (app_menu)
            set_app_menu(app_menu);
        else
            std::cerr << "MonglApp::on_startup(): No \"appmenu\" object in app_menu.ui"
                << std::endl;
    } catch (const Glib::Error& ex) {
        std::cerr << "MonglApp::on_startup(): " << ex.what() << std::endl;
        return;
    }
}

int main(int argc, char** argv) {
    auto app = MonglApp(argc, argv);

    return app.run();
}
