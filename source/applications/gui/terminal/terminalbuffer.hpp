/* Terminal application
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

#ifndef _TERMINAL_TERMINALBUFFER_HPP_
#define _TERMINAL_TERMINALBUFFER_HPP_

#include <string>
#include <vector>

struct TerminalAttribute {
    uint8_t m_bgColor;
    uint8_t m_fgColor;
};

class TerminalLine {
  public:
    TerminalLine(void);

    bool setWidth(int width);

    std::string m_text;
    std::vector<TerminalAttribute> m_attr;
    int m_dirtyWidth;
}; /* class TerminalLine */

class TerminalBuffer {
  public:
    TerminalBuffer(int width, int height);
    ~TerminalBuffer(void);

    int getLineCount(void);
    inline int getWidth(void) { return m_width; }
    inline int getHeight(void) { return m_height; }

    TerminalLine* lineAt(int index);

    bool setSize(int width, int height);

    void insertCr(void);
    void insertLf(void);
    void insertBackSpace(void);
    void insertCharacter(uint8_t c);

  private:
    int m_width;
    int m_height;

    int m_cursorX;
    int m_cursorY;
    int m_savedCursorX;
    int m_savedCursorY;

    TerminalAttribute m_attrib;
    TerminalAttribute m_savedAttrib;

    TerminalLine** m_lines;
}; /* class TerminalBuffer */

#endif /* _TERMINAL_TERMINALBUFFER_HPP_ */
