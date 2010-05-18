/* yaosp GUI library
 *
 * Copyright (c) 2010 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <assert.h>

#include <iostream>

#include <ygui++/menu.hpp>
#include <ygui++/menuitem.hpp>

namespace yguipp {

MenuBar::MenuBar( void ) : m_activeItem(NULL) {
}

MenuBar::~MenuBar( void ) {
}

void MenuBar::addChild( Widget* child, layout::LayoutData* data ) {
    MenuItem* item;

    item = dynamic_cast<MenuItem*>(child);
    std::cout << (void*)item << std::endl;
    if ( item == NULL ) {
        return;
    }

    Widget::addChild(child);
    item->setMenuParent(this);
}

Point MenuBar::getPreferredSize( void ) {
    Point size;

    for ( ChildVectorCIter it = m_children.begin();
          it != m_children.end();
          ++it ) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        size.m_x += childSize.m_x;
        size.m_y = std::max(size.m_y, childSize.m_y);
    }

    return size;
}

int MenuBar::validate( void ) {
    Point position;

    for ( ChildVectorCIter it = m_children.begin();
          it != m_children.end();
          ++it ) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        child->setPosition(position);
        child->setSize(childSize);

        position.m_x += childSize.m_x;
    }

    return 0;
}

int MenuBar::paint( GraphicsContext* g ) {
    return 0;
}

int MenuBar::itemActivated( MenuItem* item ) {
    assert(m_activeItem == NULL);
    m_activeItem = item;
    m_activeItem->setActive(true);

    return 0;
}

int MenuBar::itemDeActivated( MenuItem* item ) {
    assert(m_activeItem == item);
    m_activeItem->setActive(false);
    m_activeItem = NULL;

    return 0;
}

Menu::Menu( void ) : m_activeItem(NULL) {
    m_window = new Window("", Point(0,0), Point(0,0), WINDOW_NO_BORDER);
    m_window->init();
}

Menu::~Menu( void ) {
    delete m_window;
}

void Menu::addItem( MenuItem* item ) {
    m_window->getContainer()->addChild(item);
    item->setMenuParent(this);
}

void Menu::show( const Point& p ) {
    m_window->resize(getPreferredSize());
    m_window->moveTo(p);
    doLayout();
    m_window->show();
}

int Menu::itemActivated( MenuItem* item ) {
    assert(m_activeItem == NULL);
    m_activeItem = item;
    m_activeItem->setActive(true);

    return 0;
}

int Menu::itemDeActivated( MenuItem* item ) {
    assert(m_activeItem == item);
    m_activeItem->setActive(false);
    m_activeItem = NULL;

    return 0;
}

Point Menu::getPreferredSize( void ) {
    Point size;
    const Widget::ChildVector& children = m_window->getContainer()->getChildren();

    for ( Widget::ChildVectorCIter it = children.begin();
          it != children.end();
          ++it ) {
        Point childSize = it->first->getPreferredSize();

        size.m_x = std::max(size.m_x, childSize.m_x);
        size.m_y += childSize.m_y;
    }

    return size;
}

void Menu::doLayout( void ) {
    Point position;
    Point menuSize = m_window->getSize();
    const Widget::ChildVector& children = m_window->getContainer()->getChildren();

    for ( Widget::ChildVectorCIter it = children.begin();
          it != children.end();
          ++it ) {
        Widget* child = it->first;
        Point childSize = child->getPreferredSize();

        childSize.m_x = menuSize.m_x;
        child->setPosition(position);
        child->setSize(childSize);

        position.m_y += childSize.m_y;
    }
}

} /* namespace yguipp */
