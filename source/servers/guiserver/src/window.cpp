/* GUI server
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

#include <yaosp/debug.h>
#include <ygui++/render.hpp>

#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>

Window::Window( GuiServer* guiServer, Application* application ) : m_mouseOnDecorator(false), m_drawingMode(DM_COPY),
                                                                   m_font(NULL), m_guiServer(guiServer),
                                                                   m_application(application) {
}

Window::~Window( void ) {
}

bool Window::init( WinCreate* request ) {
    Decorator* decorator = m_guiServer->getWindowManager()->getDecorator();

    m_order = request->m_order;
    m_flags = request->m_flags;
    m_title = reinterpret_cast<char*>(request + 1);

    if (m_flags & WINDOW_NO_BORDER) {
        // todo
    } else {
        m_screenRect = yguipp::Rect(request->m_size + decorator->getSize());
        m_screenRect += request->m_position;

        m_clientRect = yguipp::Rect(request->m_size);
        m_clientRect += request->m_position;
        m_clientRect += decorator->leftTop();
    }

    m_bitmap = new Bitmap(m_screenRect.width(), m_screenRect.height(), CS_RGB32);
    m_decoratorData = decorator->createWindowData();

    return true;
}

int Window::handleMessage( uint32_t code, void* data, size_t size ) {
    switch ( code ) {
        case Y_WINDOW_SHOW :
            m_guiServer->getWindowManager()->registerWindow(this);
            break;

        case Y_WINDOW_HIDE :
            m_guiServer->getWindowManager()->unregisterWindow(this);
            break;

        case Y_WINDOW_RENDER :
            handleRender(
                reinterpret_cast<uint8_t*>(data) + sizeof(WinHeader),
                size - sizeof(WinHeader)
            );

            break;
    }

    return 0;
}

int Window::mouseEntered(yguipp::Point position) {
    m_mouseOnDecorator = !m_clientRect.hasPoint(position);

    if (m_mouseOnDecorator) {
    } else {
    }

    return 0;
}

int Window::mouseExited(void) {
    if (m_mouseOnDecorator) {
    } else {
    }

    return 0;
}

Window* Window::createFrom( GuiServer* guiServer, Application* application, WinCreate* request ) {
    Window* window = new Window(guiServer, application);
    window->init(request);
    return window;
}

void Window::handleRender( uint8_t* data, size_t size ) {
    uint8_t* dataEnd = data + size;

    while (data < dataEnd) {
        yguipp::RenderHeader* header;

        header = reinterpret_cast<yguipp::RenderHeader*>(data);

        switch (header->m_cmd) {
            case yguipp::R_SET_PEN_COLOR :
                m_penColor = reinterpret_cast<yguipp::RSetPenColor*>(header)->m_penColor;
                data += sizeof(yguipp::RSetPenColor);
                break;

            case yguipp::R_SET_FONT :
                // todo
                data += sizeof(yguipp::RSetFont);
                break;

            case yguipp::R_SET_CLIP_RECT :
                m_clipRect = reinterpret_cast<yguipp::RSetClipRect*>(header)->m_clipRect;

                if ( (m_flags & WINDOW_NO_BORDER) == 0 ) {
                    m_clipRect += m_guiServer->getWindowManager()->getDecorator()->leftTop();
                }

                data += sizeof(yguipp::RSetClipRect);
                break;

            case yguipp::R_SET_DRAWING_MODE :
                // todo
                data += sizeof(yguipp::RSetDrawingMode);
                break;

            case yguipp::R_DRAW_RECT :
                // todo
                data += sizeof(yguipp::RDrawRect);
                break;

            case yguipp::R_FILL_RECT : {
                yguipp::Rect rect = reinterpret_cast<yguipp::RFillRect*>(data)->m_rect;

                if ( (m_flags & WINDOW_NO_BORDER) == 0 ) {
                    rect += m_guiServer->getWindowManager()->getDecorator()->leftTop();
                }

                m_guiServer->getGraphicsDriver()->fillRect(
                    m_bitmap, m_clipRect, rect, m_penColor, DM_COPY
                );

                data += sizeof(yguipp::RFillRect);
                break;
            }

            case yguipp::R_DRAW_TEXT : {
                yguipp::RDrawText* cmd = reinterpret_cast<yguipp::RDrawText*>(data);

                if (m_font != NULL) {
                    yguipp::Point position = cmd->m_position;

                    if ( (m_flags & WINDOW_NO_BORDER) == 0 ) {
                        position += m_guiServer->getWindowManager()->getDecorator()->leftTop();
                    }

                    m_guiServer->getGraphicsDriver()->drawText(
                        m_bitmap, m_clipRect, position, m_penColor, m_font,
                        reinterpret_cast<char*>(cmd + 1), cmd->m_length
                    );

                }

                data += sizeof(yguipp::RDrawText);
                data += cmd->m_length;
                break;
            }

            case yguipp::R_DONE :
                m_guiServer->getWindowManager()->updateWindowRegion(this, m_screenRect);
                data += sizeof(yguipp::RenderHeader);
                break;

            default :
                dbprintf( "Window::handleRender(): unknown command: %d\n", (int)header->m_cmd );
                return;
        }
    }
}
