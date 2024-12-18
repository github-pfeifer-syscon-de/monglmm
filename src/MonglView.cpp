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
#include <GenericGlmCompat.hpp>
#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <Log.hpp>
#include <TreeGeometry2.hpp>
#include <Font2.hpp>

#include "Monitor.hpp"
#include "MonglView.hpp"
#include "TempMonitor.hpp"
#include "MemMonitor.hpp"
#include "CpuMonitor.hpp"
#include "DiskMonitor.hpp"
#include "GpuMonitor.hpp"
#include "NetMonitor.hpp"
#include "ClkMonitor.hpp"
#include "GraphShaderContext.hpp"
#include "InfoPage.hpp"
#include "Processes.hpp"
#include "StringUtils.hpp"
#include "ProcessProperties.hpp"


MonglView::MonglView(Gtk::Application* application)
: Scene()
, m_graph_shaderContext{nullptr}
, m_textContext{nullptr}
, m_updateInterval{MIN_UPDATE_PERIOD}
, m_glibtop{nullptr}
, m_diagrams()
, m_config{nullptr}
, m_Dispatcher()
, m_processes{n_values}
, m_popupMenu()
, m_filesyses{std::make_shared<Filesyses>()}
, m_diskInfos{std::make_shared<DiskInfos>()}
, m_application{application}
, m_log{psc::log::Log::create("monglmm")}
{

    read_config();
    if (m_config != nullptr) {
        config_setting_lookup_int(m_config, CONFIG_GRP_MAIN, CONFIG_UPDATE_INTERVAL,
                                  &m_updateInterval);
        if (!config_setting_lookup_color(m_config, CONFIG_GRP_MAIN, CONFIG_TEXT_COLOR,
                                  m_text_color)) {
            m_text_color = Gdk::RGBA(TEXT_DEFAULT_COLOR);
        }
        if (!config_setting_lookup_color(m_config, CONFIG_GRP_MAIN, CONFIG_BACKGOUNDCOLOR,
                                  m_background_color)) {
            m_background_color = Gdk::RGBA(BACKGROUND_DEFAULT_COLOR);
        }
        Glib::ustring uProcessType;
        if (config_setting_lookup_string(m_config, CONFIG_GRP_MAIN, CONFIG_PROCESSTYPE,
                                  uProcessType)) {
            m_processes.setTreeType(uProcessType);
        }
        Glib::ustring uLogLevel;
        if (config_setting_lookup_string(m_config, CONFIG_GRP_MAIN, CONFIG_LOGLEVEL,
                                  uLogLevel)) {
            m_log->setLevel(psc::log::Log::getLevel(uLogLevel));
        }

    }

#ifdef LIBGTOP
    m_glibtop = glibtop_init();
#endif

    m_Dispatcher.connect(sigc::mem_fun(*this, &MonglView::on_notification_from_worker_thread));

    create_popup();
}



MonglView::~MonglView()
{
    close();
    if (m_config != nullptr)
        delete m_config;
    m_config = nullptr;
}

Position
MonglView::getIntialPosition()
{
    return Position(0.0f, 4.4f, 8.0f);
}

Rotational
MonglView::getInitalAngleDegree()
{
    return Rotational(-25.0f, 0.0f, 0.0f);
}

gboolean
MonglView::init_shaders(Glib::Error &error)
{
    //printf("OpenGL information:\n");
    //printf("  version: \"%s\"\n", glGetString(GL_VERSION));
    //printf("  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    //printf("  vendor: \"%s\"\n", glGetString(GL_VENDOR));
    //printf("  renderer: \"%s\"\n", glGetString(GL_RENDERER));
    //printf("  extensions: \"%s\"\n", (char *) glGetString(GL_EXTENSIONS));
    //printf("===================================\n");

    GraphShaderContext *gctx = new GraphShaderContext();
    gboolean ret = TRUE;
    try {                                                                               // passthrough-   sampling-
        gsize szVertex,szFragm;
        Glib::RefPtr<const Glib::Bytes> refVertex = Gio::Resource::lookup_data_global(
                m_application->get_resource_base_path() + "/mongl-vertex.glsl");
        Glib::RefPtr<const Glib::Bytes> refFragm = Gio::Resource::lookup_data_global(
                m_application->get_resource_base_path() + "/mongl-fragment.glsl");
		//printf("Vertex %s\n", (char*)refVertex->get_data(szVertex));
		//printf("Fragment %s\n", (char*)refFragm->get_data(szFragm));

		std::string vertexVersioned, fragmVersioned;
		#ifndef USE_GLES	// keep shaders compatbile, thats the main benefit of es 3.0
		vertexVersioned = "#version 130\n";
		fragmVersioned = "#version 150 core\n";
		#else
		vertexVersioned = "#version 300 es\n";
		fragmVersioned = " #version 300 es\n"
			"precision mediump float;\n";
		#endif
		const char* sVertex = (const char*)refVertex->get_data(szVertex);
		vertexVersioned += sVertex;
		const char* sFragm = (const char*)refFragm->get_data(szFragm);
		fragmVersioned += sFragm;
        ret = gctx->createProgram(vertexVersioned.c_str(), fragmVersioned.c_str(), error);
    }
    catch (Glib::Error &err) {
        m_log->error(Glib::ustring::sprintf("init shader error %d %s loading resources!",  err.code(), err.what()));
        ret = FALSE;
    }
    if (!ret) {
        return FALSE;
    }
    m_graph_shaderContext = gctx;
    m_textContext = new TextContext(GL_TRIANGLES);
    if (!m_textContext->createProgram(error)) {
        return FALSE;
    }
    m_netInfo = std::make_shared<NetInfo>();

    return TRUE;
}

gboolean
MonglView::monitors_update()
{
    /* we need to ensure that the GdkGLContext is set before calling GL API */
    naviGlArea->make_current();

    for (auto d : m_diagrams) {
        d->update(m_updateInterval, m_glibtop);
    }
    m_processes.update(m_diagrams[0]->getMonitor(), m_diagrams[1]->getMonitor());

    if (m_diskInfos) {
        m_diskInfos->update(m_updateInterval, m_glibtop);
    }
    // update after disk as it depends on it
    if (m_filesyses) {
        m_filesyses->update(m_graph_shaderContext, m_textContext, m_font2, m_projView, m_updateInterval);
    }
    bool showNetConnections = config_setting_lookup_boolean(m_config, CONFIG_GRP_MAIN, CONFIG_SHOW_NET_CONNECT, true);
    if (m_netInfo && showNetConnections) {
        m_netInfo->update();
    }
    naviGlArea->queue_render();
#ifdef LIBG15
    worker->refresh();   // sync these updates
#endif

	// try to solve ram mistery
	//m_processes.printInfo();
	//auto memDiag = m_diagrams[1];
	//auto memMonitor = dynamic_cast<MemMonitor*>(memDiag->getMonitor().get());
	//if (memMonitor) {
	//	memMonitor->printInfo();
	//}
    // needs also <malloc.h>
    // Might be useful http://man7.org/linux/man-pages/man3/malloc_info.3.html
    // https://udrepper.livejournal.com/20948.html
    //malloc_info(0, stdout);

    return true;
}


void
MonglView::on_notification_from_worker_thread()
{
#ifdef LIBG15
    auto tmp = Glib::ustring::sprintf("Error %s Init %d cmd %d"
                    , worker->getCmd(), worker->getErrorCode(), worker->getCmdErrorCode());
    m_log->error(tmp);
    Gtk::MessageDialog msg = Gtk::MessageDialog(tmp, false, Gtk::MessageType::MESSAGE_ERROR);
    msg.run();
#endif
}

void
MonglView::update_timer()
{
    if (m_timer.connected())
        m_timer.disconnect(); // No more updating

    m_timer = Glib::signal_timeout().connect_seconds(
                    sigc::mem_fun(*this, &MonglView::monitors_update), m_updateInterval);
    m_log->info(Glib::ustring::sprintf("Using update interval %d", m_updateInterval));
}

void
MonglView::init(Gtk::GLArea *glArea)
{
    naviGlArea = (NaviGlArea *)glArea;

    Color clear;
    clear.r = m_background_color.get_red();
    clear.g = m_background_color.get_green();
    clear.b = m_background_color.get_blue();
    naviGlArea->set_clear_color(clear);
    m_font2 = std::make_shared<psc::gl::Font2>("sans-serif");

    /* initialize the monitors */
    std::vector<std::shared_ptr<Monitor>> graphs;
    std::shared_ptr<Monitor> cpu = std::make_shared<CpuMonitor>(n_values);
    graphs.push_back(cpu);
    std::shared_ptr<Monitor> mem = std::make_shared<MemMonitor>(n_values);
    graphs.push_back(mem);
    std::shared_ptr<Monitor> net = std::make_shared<NetMonitor>(n_values);
    graphs.push_back(net);
    std::shared_ptr<DiskMonitor> diskMonitor = std::make_shared<DiskMonitor>(n_values);
    diskMonitor->setDiskInfos(m_diskInfos);
    graphs.push_back(diskMonitor);
    std::shared_ptr<Monitor> gpu = std::make_shared<GpuMonitor>(n_values, naviGlArea);
    graphs.push_back(gpu);
    std::shared_ptr<Monitor> clk = std::make_shared<ClkMonitor>(n_values);
    graphs.push_back(clk);
    m_filesyses->setDiskInfos(m_diskInfos);
#if defined(LMSENSORS) || defined(RASPI)
    m_temp = std::make_shared<TempMonitor>(n_values);
    graphs.push_back(m_temp);
#endif

    /* initialize the diagram  */
    Position pos(-1.0f, 3.8f, 0.0f);
    for (auto m : graphs) {
        if (m_config != nullptr) {
            m->load_settings(m_config);
        }
        std::shared_ptr<DiagramMonitor> d = std::make_shared<DiagramMonitor>(m, m_graph_shaderContext, m_textContext);
        m_graph_shaderContext->addGeometry(d->getBase());
        m_diagrams.push_back(d);
        d->setFont(m_font2);
        d->setPosition(pos);
        Glib::ustring sname = m->getDisplayName();
        d->setName(sname);
        pos.y -= d->getDiagramHeight() + DIAGRAM_GAP;
    }

#ifdef LIBG15
    // As we want to listen to keys start thread ...
    m_WorkerThread = new std::thread([this, graphs]
    {
        std::vector<std::shared_ptr<Page>> pages;
        std::shared_ptr<Page> info(new InfoPage());
        pages.push_back(info);
        pages.push_back(m_filesyses);
        for (auto g : graphs) {
            pages.push_back(g);
        }

        worker = new G15Worker(pages, &m_Dispatcher);
        if (m_config != nullptr) {
            worker->read_config(m_config);
        }
        worker->setLog(m_log);
        worker->do_work();
    });
#endif

    monitors_update();      // update immediate to get faster display, drawback longer startup

    update_timer();

    /* set the window title */
    //  const char *renderer = (char *) glGetString (GL_RENDERER);
    //  char *title = g_strdup_printf ("monGLmm on %s", renderer ? renderer : "Unknown");
    //  gtk_window_set_title (GTK_WINDOW(gtk_widget_get_toplevel(*this)), title);
    //  g_free (title);
}

void
MonglView::unrealize()
{
    if (m_timer.connected()) {
        m_timer.disconnect(); // No more updating
    }
    /* we need to ensure that the GdkGLContext is set before calling GL API */
#ifdef LIBG15
    if (m_config) {
        worker->save_config(m_config);
    }
#endif
    for (auto d : m_diagrams) {
        d->close();
    }
    m_diagrams.clear();
    m_diskInfos->removeDiskInfos();
    m_filesyses.reset();
    m_diskInfos.reset();
    m_netInfo.reset();
    // This belongs to the destructor, but then we have no widget to reference GLContext from
    if (m_graph_shaderContext != nullptr) {
        delete m_graph_shaderContext;
        m_graph_shaderContext = nullptr;
    }
    if (m_textContext) {
        delete m_textContext;
        m_textContext = nullptr;
    }
}

static Glib::ustring
get_config_name()
{
    return Glib::canonicalize_filename("mongl.conf", g_get_user_config_dir());
}


void
MonglView::read_config()
{
    m_config = new Glib::KeyFile();
    auto gcfg = get_config_name();
    try {
        if (!m_config->load_from_file(gcfg, Glib::KEY_FILE_NONE)) {
            m_log->error(Glib::ustring::sprintf("Error loading %s", gcfg));
        }
    }
    catch (const Glib::FileError& exc) {
        // may happen if didn't create a file (yet) but we can go on
        m_log->error(Glib::ustring::sprintf("File Error %s loading %s if its missing, it will be created", exc.what(), gcfg));
    }
}

void
MonglView::save_config()
{
    if (m_config) {
#ifdef LIBG15
        worker->save_config(m_config);
#endif
        for (auto d : m_diagrams) {
            d->save_settings(m_config);
        }
        auto gcfg = get_config_name();
        try {
            m_config->save_to_file(gcfg);
        }
        catch (const Glib::FileError& exc) {
            auto msg = Glib::ustring::sprintf("File error %s saving config %s", exc.what(), gcfg);
            showMessage(msg, Gtk::MessageType::MESSAGE_ERROR);
        }
    }
}

Glib::KeyFile*
MonglView::getConfig()
{
    return m_config;
}

gint MonglView::getUpdateInterval()
{
    return m_updateInterval;
}

void
MonglView::drawContent()
{
    if (m_textContext) {
        m_textContext->use();
        Color text;
        text.r = m_text_color.get_red();
        text.g = m_text_color.get_green();
        text.b = m_text_color.get_blue();
        m_textContext->setColor(text);  // push color
        m_textContext->unuse();
    }

    if (m_graph_shaderContext != nullptr) {
        /* load our program */
        m_graph_shaderContext->use();
        psc::gl::checkError("useProgram");

        //glCullFace(GL_BACK);  // seems that not all triangles are correct, but as we don't have many...
        //glEnable(GL_CULL_FACE);
        m_projView = naviGlArea->getProjection() * naviGlArea->getView();

        m_graph_shaderContext->setLight();
        if (m_netInfo) {
            bool showNetConnections = config_setting_lookup_boolean(m_config, CONFIG_GRP_MAIN, CONFIG_SHOW_NET_CONNECT, true);
            m_netInfo->draw(m_graph_shaderContext, m_textContext, m_font2, showNetConnections);
        }
        // update after processe as it depends on it
        m_processes.display(m_graph_shaderContext, m_textContext, m_font2, m_diagrams[0], m_diagrams[1], m_projView);

//        if (m_filesyses) {
//            auto geos = m_filesyses->getGeometries();
//            for (auto& geo : geos) {
//                auto lgeo = geo.lease();
//                if (lgeo) {
//                    lgeo->display(m_graph_shaderContext, m_projView);
//                }
//            }
//        }
//        for (auto& d : m_diagrams) {
//            if (d) {
//                auto base = d->getBase();
//                auto lbase = base.lease();
//                if (lbase) {
//                    lbase->display(m_graph_shaderContext, m_projView);
//                }
//            }
//        }
//
//

        m_graph_shaderContext->display(m_projView);

        psc::gl::checkError("display process");

        m_graph_shaderContext->unuse();
    }
    else {
        m_log->error("No shader context on display");
    }
}

void
MonglView::draw(Gtk::GLArea *glArea, Matrix &proj, Matrix &view)
{
    drawContent();

    /* flush the contents of the pipeline */
    //glFlush();
}

void
MonglView::update_interval_changed(Gtk::SpinButton* refresh_spin)
{
    m_updateInterval = refresh_spin->get_value_as_int();
    config_group_set_int(m_config, CONFIG_GRP_MAIN, CONFIG_UPDATE_INTERVAL, m_updateInterval);
    update_timer();
}

void
MonglView::text_color_changed(Gtk::ColorButton* text_color)
{
    m_text_color = text_color->get_rgba();
    config_group_set_color(m_config, CONFIG_GRP_MAIN, CONFIG_TEXT_COLOR, m_text_color);

    naviGlArea->queue_render();
}

void
MonglView::background_color_changed(Gtk::ColorButton* background_color)
{
    m_background_color = background_color->get_rgba();
    config_group_set_color(m_config, CONFIG_GRP_MAIN, CONFIG_BACKGOUNDCOLOR, m_background_color);
    Color clear;
    clear.r = m_background_color.get_red();
    clear.g = m_background_color.get_green();
    clear.b = m_background_color.get_blue();
    naviGlArea->set_clear_color(clear);        // this might be to late but we need to use current context

    naviGlArea->queue_render();
}

void
MonglView::process_type_changed(Gtk::ComboBoxText* process_type)
{
    Glib::ustring procType(process_type->get_active_id());

    config_group_set_string(m_config, CONFIG_GRP_MAIN, CONFIG_PROCESSTYPE, procType);
    m_processes.setTreeType(procType);

    naviGlArea->queue_render();
}

void
MonglView::net_connections_show_changed(Gtk::CheckButton* showNetConn)
{
    bool bshowNetConn = showNetConn->get_active();
    if (m_config) {
        m_config->set_boolean(CONFIG_GRP_MAIN, CONFIG_SHOW_NET_CONNECT, bshowNetConn);
    }
    naviGlArea->queue_render();
}

Gtk::Dialog *
MonglView::monitors_config()
{
    Gtk::Dialog *dlg = new Gtk::Dialog("Resource monitors", Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_DESTROY_WITH_PARENT);

    //dlg->add_button(Gtk::BuiltinStockID::CLOSE);
    Gtk::Box *c_area = dlg->get_content_area();

    auto notebook = Gtk::manage(new Gtk::Notebook());
    c_area->pack_start(*notebook, TRUE, TRUE, 0);
    auto general_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0));
    notebook->append_page(*general_box, "General", FALSE);

    auto refresh_spin = Gtk::manage(new Gtk::SpinButton());
    refresh_spin->set_increments(1, 10);
    refresh_spin->set_range(MIN_UPDATE_PERIOD, MAX_UPDATE_PERIOD);
    refresh_spin->set_value(m_updateInterval);
    Monitor::add_widget2box(general_box, "Update interval (s)", refresh_spin, 0.0f);
    refresh_spin->signal_changed().connect(sigc::bind<Gtk::SpinButton *>(
                                           sigc::mem_fun(*this, &MonglView::update_interval_changed),
                                           refresh_spin));

    auto background_color_button = Gtk::manage(new Gtk::ColorButton());
    background_color_button->set_rgba(m_background_color);
    Monitor::add_widget2box(general_box, "Background color", background_color_button, 0.0f);
    background_color_button->signal_color_set().connect(
                                            sigc::bind<Gtk::ColorButton *>(
                                            sigc::mem_fun(*this, &MonglView::background_color_changed),
                                            background_color_button));

    auto text_color_button = Gtk::manage(new Gtk::ColorButton());
    text_color_button->set_rgba(m_text_color);
    Monitor::add_widget2box(general_box, "Text color", text_color_button, 0.0f);
    text_color_button->signal_color_set().connect(
                                            sigc::bind<Gtk::ColorButton *>(
                                            sigc::mem_fun(*this, &MonglView::text_color_changed),
                                            text_color_button));
    auto process_type = Gtk::manage(new Gtk::ComboBoxText());
    process_type->append("a", "Arc");
    process_type->append("b", "Block");
    process_type->append("l", "Line");
    Monitor::add_widget2box(general_box, "Process display", process_type, 0.0f);
    Glib::ustring uProcessType;
    if (config_setting_lookup_string(m_config, CONFIG_GRP_MAIN, CONFIG_PROCESSTYPE,
                              uProcessType)) {
        process_type->set_active_id(uProcessType);
    }
    else {
        process_type->set_active(0);
    }
    process_type->signal_changed().connect(sigc::bind<Gtk::ComboBoxText *>(
                                            sigc::mem_fun(*this, &MonglView::process_type_changed),
                                            process_type));


    for (auto d : m_diagrams) {
        Gtk::Box* box = d->create_config_page(this);
        notebook->append_page(*box, d->getMonitor()->m_name, FALSE);
    }
#ifdef LIBG15
    if (worker->getErrorCode() == 0) {
        Gtk::Box* g15_box = worker->create_config();
        notebook->append_page(*g15_box, "G15", FALSE);
    }
#endif

    notebook->show_all();

    return dlg;
}

bool
MonglView::on_click(GdkEventButton* event, float mx, float my)
{
    if (event->type == GDK_2BUTTON_PRESS) {
        if (m_popupMenu.get_attach_widget() == nullptr) {
            m_popupMenu.attach_to_widget(*naviGlArea);
        }
        m_popupMenu.popup_at_pointer((GdkEvent*)event);
        return true;
    }
    else {
        return Scene::on_click(event, mx, my);
    }
    return false;
}

void
MonglView::create_popup()
{
    // loading from resources did not work on connecting events ....
    //   and this is suggested by https://developer.gnome.org/gtkmm-tutorial/stable/sec-treeview-examples.html.en
    auto itemProp = Gtk::make_managed<Gtk::MenuItem>("_Properties", true);
    itemProp->signal_activate().connect(
            sigc::mem_fun(*this, &MonglView::on_process_properties) );
    m_popupMenu.append(*itemProp);

    auto itemRestore = Gtk::make_managed<Gtk::MenuItem>("_Restore", true);
    itemRestore->signal_activate().connect(
            sigc::mem_fun(*this, &MonglView::restore) );
    m_popupMenu.append(*itemRestore);

    m_popupMenu.show_all();
}

void
MonglView::on_process_properties()
{
    auto selectGeom2 = getSelected();
    if (selectGeom2) {
        auto treeGeom = psc::mem::dynamic_pointer_cast<psc::gl::TreeGeometry2>(selectGeom2);
        if (treeGeom) {
            if (auto lTreeGeom = treeGeom.lease()) {
                std::shared_ptr<psc::gl::TreeNode2> treeNode{lTreeGeom->getTreeNode()};
                if (treeNode) {
                    auto process = std::dynamic_pointer_cast<Process>(treeNode);
                    if (process) {
                        ProcessProperties* procProp = ProcessProperties::show(process->getPid(), m_config, m_updateInterval);
                        if (procProp) {
                            procProp->run();
                            save_config();
                            procProp->hide();
                            delete procProp;
                        }
                    }
                    else {
                        showMessage("Works only for processes...");
                    }
                }
            }
        }
    }
    else {
        showMessage("No selection!");
    }
}

void
MonglView::showMessage(const Glib::ustring& msg, Gtk::MessageType msgType)
{
    Gtk::MessageDialog messagedialog(*m_application->get_active_window(), msg, false, msgType);
    messagedialog.run();
    messagedialog.hide();
}

void
MonglView::restore() {
    m_processes.restore();
    naviGlArea->queue_render();
}

psc::gl::aptrGeom2
MonglView::on_click_select(GdkEventButton* event, float mx, float my)
{
    auto selected = m_graph_shaderContext->hit2(mx, my);
    return selected;
}

bool
MonglView::selectionChanged(const psc::gl::aptrGeom2& prev_selected, const psc::gl::aptrGeom2& selected)
{
    if (prev_selected) {
        auto treeGeo = psc::mem::dynamic_pointer_cast<psc::gl::TreeGeometry2>(prev_selected);
        if (auto ltreeGeo = treeGeo.lease()) {
            ltreeGeo->setTextVisible(false);
        }
    }
    auto treeGeo = psc::mem::dynamic_pointer_cast<psc::gl::TreeGeometry2>(selected);
    if (auto ltreeGeo = treeGeo.lease()) {
        ltreeGeo->setTextVisible(true);
    }
    return false;
}

void
MonglView::close()
{
    // close hardware related resources
#ifdef LIBG15
    if (worker != nullptr) {
        //std::cout << "MonglView::close worker" << std::endl;
        worker->stop();
        m_WorkerThread->join();
        delete m_WorkerThread;
        delete worker;
        worker = nullptr;
    }
#endif
#ifdef LIBGTOP
    if (m_glibtop) {
        glibtop_close_r(m_glibtop);
        m_glibtop = nullptr;
    }
#endif
    if (m_temp) {
        //std::cout << "MonglView::close temp" << std::endl;
        m_temp->close();
        m_temp.reset();
    }
}
