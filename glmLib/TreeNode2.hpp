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

#include "memory"
#include "list"

#include "Geom2.hpp"

namespace psc {
namespace gl {


class TreeGeometry2;

enum class TreeNodeState {
    New = 0
  , Running
  , Finished
  , Close
};

class TreeNode2
//: public std::enable_shared_from_this<TreeNode2>
{
public:


    TreeNode2();
    virtual ~TreeNode2();

    void add(const std::shared_ptr<TreeNode2>& _child);
    void remove(TreeNode2 *_child);
    void setParent(TreeNode2 *_parent, const std::shared_ptr<TreeNode2>& sharedThis);
    virtual const char* getName() = 0;
    virtual TreeNodeState getStage() = 0;
    virtual float getLoad() = 0;
    virtual Glib::ustring getDisplayName() = 0;
    virtual bool isPrimary() = 0;
    TreeNode2 *getParent();
    std::list<std::shared_ptr<TreeNode2>>& getChildren();
    psc::mem::active_ptr<TreeGeometry2>& getTreeGeometry();
    void setTreeGeometry(const psc::mem::active_ptr<TreeGeometry2>& _treeGeometry);
    void setLeafCount(double leafCount);
    double getLeafCount();
    bool isSelected();
    void setSelected(bool selected);
    void removeGeometry();
    bool isRoot() const;
protected:
    TreeNode2 *m_parent;
    std::list<std::shared_ptr<TreeNode2>> m_children;

    psc::mem::active_ptr<TreeGeometry2> m_treeGeometry;
    double m_leafCount;
    bool m_selected;
private:
    void setTextVisible(bool textVisisble);
};



} /* namespace gl */
} /* namespace psc */