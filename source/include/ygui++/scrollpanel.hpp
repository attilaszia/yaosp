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

#ifndef _SCROLLPANEL_HPP_
#define _SCROLLPANEL_HPP_

#include <ygui++/scrollbar.hpp>
#include <ygui++/event/adjustmentlistener.hpp>

namespace yguipp {

class ScrollPanel : public Widget, event::AdjustmentListener {
  public:
    ScrollPanel(void);
    virtual ~ScrollPanel(void);

    void add(Widget* child, layout::LayoutData* data = NULL);

    int validate(void);

    int onAdjustmentValueChanged(Widget* widget);

  private:
    int verticalValueChanged(void);
    int horizontalValueChanged(void);

  private:
    ScrollBar* m_verticalBar;
    ScrollBar* m_horizontalBar;
    Widget* m_scrolledWidget;
}; /* class ScrollPanel */

} /* namespace yguipp */

#endif /* _SCROLLPANEL_HPP_ */
