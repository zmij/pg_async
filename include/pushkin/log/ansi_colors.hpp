/*
 * ansi_colors.hpp
 *
 *  Created on: Jul 14, 2015
 *      Author: zmij
 */

#ifndef PUSHKIN_UTIL_ANSI_COLORS_HPP_
#define PUSHKIN_UTIL_ANSI_COLORS_HPP_

#include <iosfwd>

namespace psst {
namespace util {

enum ANSI_COLOR {
    // clear the color
    CLEAR        = 0x00,
    // attribs
    BRIGHT        = 0x01,
    DIM            = BRIGHT * 2,
    UNDERLINE    = DIM * 2,

    FOREGROUND    = UNDERLINE * 2,
    BACKGROUND    = FOREGROUND * 2,
    // normal colors
    BLACK        = BACKGROUND * 2,
    RED            = BLACK * 2,
    GREEN        = RED * 2,
    YELLOW        = GREEN * 2,
    BLUE        = YELLOW * 2,
    MAGENTA        = BLUE * 2,
    CYAN        = MAGENTA * 2,
    WHITE        = CYAN * 2,

    // Color mask
    COLORS        = BLACK | RED | GREEN | YELLOW |
                    BLUE | MAGENTA | CYAN | WHITE
};

std::ostream&
operator << (std::ostream&, ANSI_COLOR);

inline ANSI_COLOR
operator | (ANSI_COLOR a, ANSI_COLOR b)
{
    return (ANSI_COLOR)((int)a | (int)b);
}

}  // namespace util
}  // namespace psst

#endif /* PUSHKIN_UTIL_ANSI_COLORS_HPP_ */
