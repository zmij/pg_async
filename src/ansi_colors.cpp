/*
 * ansi_colors.cpp
 *
 *  Created on: Jul 14, 2015
 *      Author: zmij
 */

#include <tip/log/ansi_colors.hpp>
#include <iostream>

namespace tip {
namespace util {

namespace {

const int multiply_de_bruijn_bit_position[32] =
{
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

int
set_bits(int i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int
lowest_bit_set(unsigned int v)
{
	return multiply_de_bruijn_bit_position[((unsigned int)((v & -v) * 0x077CB531U)) >> 27];
}

char ESC = '\033';

}  // namespace

std::ostream&
operator << (std::ostream& out, ANSI_COLOR col)
{
	std::ostream::sentry s(out);
	if (s) {
		if (col == CLEAR) {
			out << ESC << "[0m";
		} else {
			if (col & BRIGHT) {
				out << ESC << "[1m";
			} else if (col & DIM) {
				out << ESC << "[2m";
			}
			if (col & UNDERLINE) {
				out << ESC << "[4m";
			}

			char fg = '3';
			if (col & BACKGROUND) {
				fg = '4';
			}
			// clear attribute bits
			col = (ANSI_COLOR)(col & COLORS);
			if (col) {
				int color_pos = lowest_bit_set(col) - lowest_bit_set(BLACK);
				out << ESC << '[' << fg << (char)('0' + color_pos) << 'm';
			}
		}
	}
	return out;
}

}  // namespace util
}  // namespace tip
