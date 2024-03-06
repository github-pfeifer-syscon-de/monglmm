/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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
#include <map>
#include <memory>
#include <NaviContext.hpp>
#include <TextContext.hpp>
#include <Geom2.hpp>
#include <Text2.hpp>
#include <Font2.hpp>

namespace psc {
namespace gl {


class NamedTreeNode2;

typedef std::shared_ptr<NamedTreeNode2> ptrNamedTreeNode;

class NamedTreeNode2
{
public:
    NamedTreeNode2(const Glib::ustring& name, const Glib::ustring& key);
    explicit NamedTreeNode2(const NamedTreeNode2& orig) = delete;
    virtual ~NamedTreeNode2();

    Glib::ustring getDisplayName() const;
    Glib::ustring getKey() const;
    ptrNamedTreeNode getChild(const Glib::ustring& key);
    void add(const ptrNamedTreeNode& pNamedTreeNode);
    std::list<ptrNamedTreeNode> getChildren();
    aptrGeom2 getGeo();
    void createLineDown(NaviContext *shaderContext, float y);
    aptrGeom2 getTreeGeometry(NaviContext *shaderContext
            ,TextContext *txtCtx
            ,const ptrFont2& pFont
            ,aptrGeom2 pParentGeo);
    virtual bool isMarkGeometryUpdated() = 0;
    virtual void updateMarkGeometry(NaviContext *shaderContext) = 0;
    float render(NaviContext *shaderContext
                ,TextContext *txtCtx
                ,const ptrFont2& pFont
                ,const aptrGeom2& pParentGeo);
    void setParent(NamedTreeNode2* parent);
protected:
    Glib::ustring m_name;
    Glib::ustring m_key;
    std::map<Glib::ustring, ptrNamedTreeNode> m_children;
    aptrGeom2 m_geo;
    aptrGeom2 m_lineDown;
    NamedTreeNode2* m_parent{nullptr};
    float m_lastY{0.0f};
    static constexpr auto LINE_COLOR = 0.8f;
    static constexpr auto NODE_INDENT = 0.2f;
    static constexpr auto NODE_LINESPACE = -0.2f;

private:

};


} /* namespace gl */
} /* namespace psc */