/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include "TreeGeometry2.hpp"

#include "TreeNode2.hpp"

namespace psc {
namespace gl {


TreeNode2::TreeNode2()
: m_parent{nullptr}
, m_selected{false}
{
}

TreeNode2::~TreeNode2()
{
    m_treeGeometry.reset();
    m_children.clear();
}

void
TreeNode2::add(const std::shared_ptr<TreeNode2>& _child)
{
    m_children.push_back(_child);
}

void
TreeNode2::remove(TreeNode2 *_child)
{
    for (auto iter = m_children.begin(); iter != m_children.end(); ) {
        auto chld = *iter;
        if (chld.get() != _child) {
            ++iter;
        }
        else {
            _child->m_parent = nullptr;
            iter = m_children.erase(iter);
            break;
        }
    }
}

void
TreeNode2::setParent(TreeNode2 *_parent, const std::shared_ptr<TreeNode2>& sharedThis)
{
    if (m_parent == _parent) {
        return;
    }
    if (m_parent != nullptr) {
        m_parent->remove(this);
    }
    m_parent = _parent;
    if (m_parent != nullptr) {
        m_parent->add(sharedThis);      // shared_from_this()
        m_selected = m_parent->isSelected();
    }
}

void
TreeNode2::setLeafCount(double leafCount)
{
    m_leafCount = leafCount;
}

double
TreeNode2::getLeafCount()
{
    return m_leafCount;
}

void
TreeNode2::setTextVisible(bool textVisisble)
{
    auto lTreeGeo = m_treeGeometry.lease();
    if (lTreeGeo) {
        lTreeGeo->setTextVisible(textVisisble);
    }
}

void
TreeNode2::setTreeGeometry(const psc::mem::active_ptr<TreeGeometry2>& _treeGeometry)
{
    auto _ltreeGeo = _treeGeometry.lease();
    if (_ltreeGeo) {
        auto lmTreeGeo = m_treeGeometry.lease();
        if (lmTreeGeo) {
            _ltreeGeo->setPosition(lmTreeGeo->getPos());   // inherit a possibly modified transform
            _ltreeGeo->setScale(lmTreeGeo->getScale());
            _ltreeGeo->setRotation(lmTreeGeo->getRotation());
        }
    }
    //_treeGeometry->addDestructionListener(this);    // keep track if geometry will get destroyed by recursive destruction
    m_treeGeometry = _treeGeometry;
}


bool
TreeNode2::isSelected() {
    return m_selected;
}

void
TreeNode2::setSelected(bool selected) {
    m_selected = selected;
    setTextVisible(selected);
    for (auto& p_chldNode : m_children) {
        p_chldNode->setSelected(selected);
    }
}

void
TreeNode2::removeGeometry()
{
    m_treeGeometry.reset();
}


bool
TreeNode2::isRoot() const
{
    return m_parent == nullptr;
}

TreeNode2 *
TreeNode2::getParent()
{
    return m_parent;
}

std::list<std::shared_ptr<TreeNode2>>&
TreeNode2::getChildren()
{
    return m_children;
}

psc::mem::active_ptr<TreeGeometry2>&
TreeNode2::getTreeGeometry()
{
    return m_treeGeometry;
}



} /* namespace gl */
} /* namespace psc */