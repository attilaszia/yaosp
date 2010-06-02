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
#include <guiserver/graphicsdriver.hpp>

int GraphicsDriver::fillRect( Bitmap* bitmap, const yguipp::Rect& clipRect, const yguipp::Rect& rect,
                              const yguipp::Color& color, drawing_mode_t mode ) {
    yguipp::Rect rectToFill = rect;

    rectToFill &= clipRect;
    rectToFill &= bitmap->bounds();

    if ( !rectToFill.isValid() ) {
        return 0;
    }

    switch (mode) {
        case DM_COPY :
            fillRectCopy(bitmap, rectToFill, color);
            break;

        case DM_BLEND :
            break;

        case DM_INVERT :
            break;
    }

    return 0;
}

int GraphicsDriver::blitBitmap( Bitmap* dest, const yguipp::Point& point, Bitmap* src,
                                const yguipp::Rect& rect, drawing_mode_t mode ) {
    yguipp::Rect rectToBlit = rect;

    rectToBlit &= src->bounds();
    rectToBlit &= yguipp::Rect(point.m_x, point.m_y, dest->width() - 1, dest->height() - 1).bounds() + rect.leftTop();

    if ( !rectToBlit.isValid() ) {
        return 0;
    }

    switch (mode) {
        case DM_COPY :
            blitBitmapCopy(dest, point, src, rectToBlit);
            break;

        case DM_BLEND :
            blitBitmapBlend(dest, point, src, rectToBlit);
            break;

        case DM_INVERT :
            break;
    }

    return 0;
}

int GraphicsDriver::fillRectCopy( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color ) {
    switch ( (int)bitmap->getColorSpace() ) {
        case CS_RGB32 :
            fillRectCopy32(bitmap, rect, color);
            break;
    }

    return 0;
}

int GraphicsDriver::fillRectCopy32( Bitmap* bitmap, const yguipp::Rect& rect, const yguipp::Color& color ) {
    register uint32_t* data;
    uint32_t c = color.toColor32();
    uint32_t padding = bitmap->width() - rect.width();

    data = reinterpret_cast<uint32_t*>(bitmap->getBuffer()) + (rect.m_top * bitmap->width() + rect.m_left);

    for ( int y = 0; y < rect.height(); y++ ) {
        for ( int x = 0; x < rect.width(); x++ ) {
            *data++ = c;
        }

        data += padding;
    }

    return 0;
}

int GraphicsDriver::blitBitmapCopy( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect ) {
    switch ( (int)dest->getColorSpace() ) {
        case CS_RGB32 :
            blitBitmapCopy32(dest, point, src, rect);
            break;
    }

    return 0;
}

int GraphicsDriver::blitBitmapCopy32( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect ) {
    uint32_t* dst_buffer = reinterpret_cast<uint32_t*>(dest->getBuffer()) + point.m_y * dest->width() + point.m_x;

    switch ( (int)src->getColorSpace() ) {
        case CS_RGB32 : {
            uint32_t* src_buffer = reinterpret_cast<uint32_t*>(src->getBuffer()) +
                rect.m_top * src->width() + rect.m_left;

            for ( int y = 0; y < rect.height(); y++ ) {
                memcpy(dst_buffer, src_buffer, rect.width() * 4);
                dst_buffer += dest->width();
                src_buffer += src->width();
            }

            break;
        }
    }

    return 0;
}

int GraphicsDriver::blitBitmapBlend( Bitmap* dest, const yguipp::Point& point, Bitmap* src, const yguipp::Rect& rect ) {
    switch ( (int)dest->getColorSpace() ) {
        case CS_RGB32 :
            blitBitmapBlend32(dest, point, src, rect);
            break;
    }

    return 0;
}

int GraphicsDriver::blitBitmapBlend32( Bitmap* dest, const yguipp::Point& point,
                                       Bitmap* src, const yguipp::Rect& rect ) {
    int dst_modulo = dest->width() - rect.width();
    uint32_t* dst_buffer = reinterpret_cast<uint32_t*>(dest->getBuffer()) + point.m_y * dest->width() + point.m_x;

    switch ( (int)src->getColorSpace() ) {
        case CS_RGB32 : {
            int src_modulo = src->width() - rect.width();
            uint32_t* src_buffer = reinterpret_cast<uint32_t*>(src->getBuffer()) +
                rect.m_top * src->width() + rect.m_left;

            for ( int y = 0; y < rect.height(); y++ ) {
                for ( int x = 0; x < rect.width(); x++ ) {
                    register uint32_t src_color = *src_buffer++;
                    uint32_t src_alpha = src_color >> 24;

                    if ( src_alpha == 0xFF ) {
                        *dst_buffer = ( *dst_buffer & 0xFF000000 ) | ( src_color & 0x00FFFFFF );
                    } else if ( src_alpha != 0x00 ) {
                        uint32_t src1;
                        uint32_t dst1;
                        uint32_t dst_alpha;
                        register uint32_t dst_color;

                        dst_color = *dst_buffer;
                        dst_alpha = dst_color & 0xFF000000;

                        src1 = src_color & 0xFF00FF;
                        dst1 = dst_color & 0xFF00FF;
                        dst1 = ( dst1 + ( ( src1 - dst1 ) * src_alpha >> 8 ) ) & 0xFF00FF;
                        src_color &= 0xFF00;
                        dst_color &= 0xFF00;
                        dst_color = ( dst_color + ( ( src_color - dst_color ) * src_alpha >> 8 ) ) & 0xFF00;

                        *dst_buffer = dst1 | dst_color | dst_alpha;
                    }

                    dst_buffer++;
                }

                dst_buffer += dst_modulo;
                src_buffer += src_modulo;
            }

            break;
        }
    }

    return 0;
}
