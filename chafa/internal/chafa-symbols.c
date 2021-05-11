/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Copyright (C) 2018-2021 Hans Petter Jansson
 *
 * This file is part of Chafa, a program that turns images into character art.
 *
 * Chafa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chafa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Chafa.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"

#include <string.h>  /* memset */
#include "chafa.h"
#include "internal/chafa-private.h"

typedef struct
{
    gunichar first, last;
}
UnicharRange;

typedef struct
{
    ChafaSymbolTags sc;
    gunichar c;
    gchar *coverage;
}
ChafaSymbolDef;

ChafaSymbol *chafa_symbols;
static gboolean symbols_initialized;

/* Ranges we treat as ambiguous-width in addition to the ones defined by
 * GLib. For instance: VTE, although spacing correctly, has many glyphs
 * extending well outside their cells resulting in ugly overlapping. */
static const UnicharRange ambiguous_ranges [] =
{
    {  0x00ad,  0x00ad },  /* Soft hyphen */
    {  0x2196,  0x21ff },  /* Arrows (most) */

    {  0x222c,  0x2237 },  /* Mathematical ops (some) */
    {  0x2245,  0x2269 },  /* Mathematical ops (some) */
    {  0x226d,  0x2279 },  /* Mathematical ops (some) */
    {  0x2295,  0x22af },  /* Mathematical ops (some) */
    {  0x22bf,  0x22bf },  /* Mathematical ops (some) */
    {  0x22c8,  0x22ff },  /* Mathematical ops (some) */

    {  0x2300,  0x23ff },  /* Technical */
    {  0x2460,  0x24ff },  /* Enclosed alphanumerics */
    {  0x25a0,  0x25ff },  /* Geometric */
    {  0x2700,  0x27bf },  /* Dingbats */
    {  0x27c0,  0x27e5 },  /* Miscellaneous mathematical symbols A (most) */
    {  0x27f0,  0x27ff },  /* Supplemental arrows A */
    {  0x2900,  0x297f },  /* Supplemental arrows B */
    {  0x2980,  0x29ff },  /* Miscellaneous mathematical symbols B */
    {  0x2b00,  0x2bff },  /* Miscellaneous symbols and arrows */
    { 0x1f100, 0x1f1ff },  /* Enclosed alphanumeric supplement */

    { 0, 0 }
};

/* Emojis of various kinds; usually multicolored. We have no control over
 * the foreground colors of these, and they may render poorly for other
 * reasons (e.g. too wide). */
static const UnicharRange emoji_ranges [] =
{
    {  0x2600,  0x26ff },  /* Miscellaneous symbols */
    { 0x1f000, 0x1ffff },  /* Emojis */

    /* This symbol usually prints fine, but we don't want it randomly
     * popping up in our output anyway. So we add it to the "ugly" category,
     * which is excluded from "all". */
    {  0x534d,  0x534d },

    { 0, 0 }
};

static const UnicharRange meta_ranges [] =
{
    /* Ideographic description characters. These convert poorly to our
     * internal format. */
    {  0x2ff0, 0x2fff },

    { 0, 0 }
};

static const ChafaSymbolDef symbol_defs [] =
{
#include "chafa-symbols-ascii.h"
    {
        /* Horizontal Scan Line 1 */
        CHAFA_SYMBOL_TAG_TECHNICAL,
        0x23ba,
        "        "
        "XXXXXXXX"
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
    },
    {
        /* Horizontal Scan Line 3 */
        CHAFA_SYMBOL_TAG_TECHNICAL,
        0x23bb,
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "        "
        "        "
        "        "
        "        "
    },
    {
        /* Horizontal Scan Line 7 */
        CHAFA_SYMBOL_TAG_TECHNICAL,
        0x23bc,
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        /* Horizontal Scan Line 9 */
        CHAFA_SYMBOL_TAG_TECHNICAL,
        0x23bd,
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_VHALF | CHAFA_SYMBOL_TAG_INVERTED,
        0x2580,
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2581,
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2582,
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2583,
        "        "
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_VHALF,
        0x2584,
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2585,
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2586,
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2587,
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        /* Full block */
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_SOLID,
        0x2588,
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x2589,
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
        "XXXXXXX "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x258a,
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
        "XXXXXX  "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x258b,
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
        "XXXXX   "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_HHALF,
        0x258c,
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x258d,
        "XXX     "
        "XXX     "
        "XXX     "
        "XXX     "
        "XXX     "
        "XXX     "
        "XXX     "
        "XXX     "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x258e,
        "XX      "
        "XX      "
        "XX      "
        "XX      "
        "XX      "
        "XX      "
        "XX      "
        "XX      "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK,
        0x258f,
        "X       "
        "X       "
        "X       "
        "X       "
        "X       "
        "X       "
        "X       "
        "X       "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_HHALF | CHAFA_SYMBOL_TAG_INVERTED,
        0x2590,
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_INVERTED,
        0x2594,
        "XXXXXXXX"
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_INVERTED,
        0x2595,
        "       X"
        "       X"
        "       X"
        "       X"
        "       X"
        "       X"
        "       X"
        "       X"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD,
        0x2596,
        "        "
        "        "
        "        "
        "        "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD,
        0x2597,
        "        "
        "        "
        "        "
        "        "
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD,
        0x2598,
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD | CHAFA_SYMBOL_TAG_INVERTED,
        0x2599,
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD,
        0x259a,
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD | CHAFA_SYMBOL_TAG_INVERTED,
        0x259b,
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD | CHAFA_SYMBOL_TAG_INVERTED,
        0x259c,
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD,
        0x259d,
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD | CHAFA_SYMBOL_TAG_INVERTED,
        0x259e,
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "XXXX    "
        "XXXX    "
        "XXXX    "
        "XXXX    "
    },
    {
        CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_QUAD | CHAFA_SYMBOL_TAG_INVERTED,
        0x259f,
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "    XXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
        "XXXXXXXX"
    },
    /* Begin box drawing characters */
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2500,
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2501,
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2502,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2503,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2504,
        "        "
        "        "
        "        "
        "        "
        "XX XX XX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2505,
        "        "
        "        "
        "        "
        "XX XX XX"
        "XX XX XX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2506,
        "    X   "
        "    X   "
        "        "
        "    X   "
        "    X   "
        "        "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2507,
        "   XX   "
        "   XX   "
        "        "
        "   XX   "
        "   XX   "
        "        "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2508,
        "        "
        "        "
        "        "
        "        "
        "X X X X "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2509,
        "        "
        "        "
        "        "
        "X X X X "
        "X X X X "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x250a,
        "    X   "
        "        "
        "    X   "
        "        "
        "    X   "
        "        "
        "    X   "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x250b,
        "   XX   "
        "        "
        "   XX   "
        "        "
        "   XX   "
        "        "
        "   XX   "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x250c,
        "        "
        "        "
        "        "
        "        "
        "    XXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x250d,
        "        "
        "        "
        "        "
        "    XXXX"
        "    XXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x250e,
        "        "
        "        "
        "        "
        "        "
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x250f,
        "        "
        "        "
        "        "
        "   XXXXX"
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2510,
        "        "
        "        "
        "        "
        "        "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2511,
        "        "
        "        "
        "        "
        "XXXXX   "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2512,
        "        "
        "        "
        "        "
        "        "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2513,
        "        "
        "        "
        "        "
        "XXXXX   "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2514,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2515,
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "    XXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2516,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2517,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "   XXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2518,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2519,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXX   "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251a,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251b,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXX   "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251c,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251d,
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "    XXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251e,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x251f,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2520,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2521,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "   XXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2522,
        "    X   "
        "    X   "
        "    X   "
        "   XXXXX"
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2523,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "   XXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2524,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2525,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2526,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2527,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2528,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2529,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXX   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252a,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252b,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252c,
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252d,
        "        "
        "        "
        "        "
        "XXXXX   "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252e,
        "        "
        "        "
        "        "
        "    XXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x252f,
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2530,
        "        "
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2531,
        "        "
        "        "
        "        "
        "XXXXX   "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2532,
        "        "
        "        "
        "        "
        "   XXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2533,
        "        "
        "        "
        "        "
        "XXXXXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2534,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2535,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2536,
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2537,
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2538,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2539,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253a,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253b,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253c,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253d,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253e,
        "    X   "
        "    X   "
        "    X   "
        "    XXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x253f,
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2540,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2541,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2542,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2543,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2544,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2545,
        "    X   "
        "    X   "
        "    X   "
        "XXXXX   "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2546,
        "    X   "
        "    X   "
        "    X   "
        "   XXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2547,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "XXXXXXXX"
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2548,
        "    X   "
        "    X   "
        "    X   "
        "XXXXXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x2549,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXX   "
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x254a,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x254b,
        "   XX   "
        "   XX   "
        "   XX   "
        "XXXXXXXX"
        "XXXXXXXX"
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x254c,
        "        "
        "        "
        "        "
        "        "
        "XXX  XXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x254d,
        "        "
        "        "
        "        "
        "XXX  XXX"
        "XXX  XXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x254e,
        "    X   "
        "    X   "
        "    X   "
        "        "
        "        "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x254f,
        "   XX   "
        "   XX   "
        "   XX   "
        "        "
        "        "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DIAGONAL,
        0x2571,
        "       X"
        "      X "
        "     X  "
        "    X   "
        "   X    "
        "  X     "
        " X      "
        "X       "
    },
    {
        /* Variant */
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DIAGONAL,
        0x2571,
        "      XX"
        "     XXX"
        "    XXX "
        "   XXX  "
        "  XXX   "
        " XXX    "
        "XXX     "
        "XX      "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DIAGONAL,
        0x2572,
        "X       "
        " X      "
        "  X     "
        "   X    "
        "    X   "
        "     X  "
        "      X "
        "       X"
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DIAGONAL,
        0x2572,
        "XX      "
        "XXX     "
        " XXX    "
        "  XXX   "
        "   XXX  "
        "    XXX "
        "     XXX"
        "      XX"
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DIAGONAL,
        0x2573,
        "X      X"
        " X    X "
        "  X  X  "
        "   XX   "
        "   XX   "
        "  X  X  "
        " X    X "
        "X      X"
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2574,
        "        "
        "        "
        "        "
        "        "
        "XXXX    "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2575,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2576,
        "        "
        "        "
        "        "
        "        "
        "    XXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2577,
        "        "
        "        "
        "        "
        "        "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2578,
        "        "
        "        "
        "        "
        "XXXX    "
        "XXXX    "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x2579,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "        "
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x257a,
        "        "
        "        "
        "        "
        "    XXXX"
        "    XXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER | CHAFA_SYMBOL_TAG_DOT,
        0x257b,
        "        "
        "        "
        "        "
        "        "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x257c,
        "        "
        "        "
        "        "
        "    XXXX"
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x257d,
        "    X   "
        "    X   "
        "    X   "
        "    X   "
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x257e,
        "        "
        "        "
        "        "
        "XXXX    "
        "XXXXXXXX"
        "        "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_BORDER,
        0x257f,
        "   XX   "
        "   XX   "
        "   XX   "
        "   XX   "
        "    X   "
        "    X   "
        "    X   "
        "    X   "
    },
    /* Begin dot characters*/
    {
        CHAFA_SYMBOL_TAG_DOT,
        0x25ae, /* Black vertical rectangle */
        "        "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_DOT,
        0x25a0, /* Black square */
        "        "
        "        "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        "        "
        "        "
    },
    {
        /* Has an emoji variant that may show up unbidden */
        CHAFA_SYMBOL_TAG_DOT | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25aa, /* Black small square */
        "        "
        "        "
        "  XXXX  "
        "  XXXX  "
        "  XXXX  "
        "  XXXX  "
        "        "
        "        "
    },
    {
        /* Black up-pointing triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC,
        0x25b2,
        "        "
        "   XX   "
        "  XXXX  "
        " XXXXXX "
        " XXXXXX "
        "XXXXXXXX"
        "        "
        "        "
    },
    {
        /* Black right-pointing triangle */
        /* Has an emoji variant that may show up unbidden */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25b6,
        " X      "
        " XXX    "
        " XXXX   "
        " XXXXXX "
        " XXXX   "
        " XXX    "
        " X      "
        "        "
    },
    {
        /* Black down-pointing triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC,
        0x25bc,
        "        "
        "XXXXXXXX"
        " XXXXXX "
        " XXXXXX "
        "  XXXX  "
        "   XX   "
        "        "
        "        "
    },
    {
        /* Black left-pointing triangle */
        /* Has an emoji variant that may show up unbidden */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25c0,
        "      X "
        "    XXX "
        "   XXXX "
        " XXXXXX "
        "   XXXX "
        "    XXX "
        "      X "
        "        "
    },
    {
        /* Black diamond */
        /* Depending on font, may exceed cell boundaries on VTE */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25c6,
        "        "
        "   XX   "
        "  XXXX  "
        " XXXXXX "
        "  XXXX  "
        "   XX   "
        "        "
        "        "
    },
    {
        /* Black Circle */
        CHAFA_SYMBOL_TAG_GEOMETRIC,
        0x25cf,
        "        "
        "  XXXX  "
        " XXXXXX "
        " XXXXXX "
        " XXXXXX "
        "  XXXX  "
        "        "
        "        "
    },
    {
        /* Black Lower Right Triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25e2,
        "        "
        "        "
        "     XX "
        "    XXX "
        "   XXXX "
        " XXXXXX "
        "        "
        "        "
    },
    {
        /* Black Lower Left Triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25e3,
        "        "
        "        "
        " XX     "
        " XXX    "
        " XXXX   "
        " XXXXXX "
        "        "
        "        "
    },
    {
        /* Black Upper Left Triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25e4,
        "        "
        "        "
        " XXXXXX "
        " XXXX   "
        " XXX    "
        " XX     "
        "        "
        "        "
    },
    {
        /* Black Upper Right Triangle */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25e5,
        "        "
        "        "
        " XXXXXX "
        "   XXXX "
        "    XXX "
        "     XX "
        "        "
        "        "
    },
    {
        /* Black Medium Square */
        /* Has en emoji variant that may show up unbidden. See:
         * https://github.com/hpjansson/chafa/issues/52 */
        CHAFA_SYMBOL_TAG_GEOMETRIC | CHAFA_SYMBOL_TAG_AMBIGUOUS,
        0x25fc,
        "        "
        "        "
        "  XXXX  "
        "  XXXX  "
        "  XXXX  "
        "  XXXX  "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_DOT,
        0x00b7, /* Middle dot */
        "        "
        "        "
        "        "
        "   XX   "
        "   XX   "
        "        "
        "        "
        "        "
    },
    {
        /* Variant */
        CHAFA_SYMBOL_TAG_DOT,
        0x00b7, /* Middle dot */
        "        "
        "        "
        "        "
        "  XX    "
        "  XX    "
        "        "
        "        "
        "        "
    },
    {
        /* Variant */
        CHAFA_SYMBOL_TAG_DOT,
        0x00b7, /* Middle dot */
        "        "
        "        "
        "        "
        "    XX  "
        "    XX  "
        "        "
        "        "
        "        "
    },
    {
        /* Variant */
        CHAFA_SYMBOL_TAG_DOT,
        0x00b7, /* Middle dot */
        "        "
        "        "
        "   XX   "
        "   XX   "
        "        "
        "        "
        "        "
        "        "
    },
    {
        /* Variant */
        CHAFA_SYMBOL_TAG_DOT,
        0x00b7, /* Middle dot */
        "        "
        "        "
        "        "
        "        "
        "   XX   "
        "   XX   "
        "        "
        "        "
    },
    {
        CHAFA_SYMBOL_TAG_STIPPLE,
        0x2591,
        "X   X   "
        "  X   X "
        "X   X   "
        "  X   X "
        "X   X   "
        "  X   X "
        "X   X   "
        "  X   X "
    },
    {
        CHAFA_SYMBOL_TAG_STIPPLE,
        0x2592,
        "X X X X "
        " X X X X"
        "X X X X "
        " X X X X"
        "X X X X "
        " X X X X"
        "X X X X "
        " X X X X"
    },
    {
        CHAFA_SYMBOL_TAG_STIPPLE,
        0x2593,
        " XXX XXX"
        "XX XXX X"
        " XXX XXX"
        "XX XXX X"
        " XXX XXX"
        "XX XXX X"
        " XXX XXX"
        "XX XXX X"
    },
    {
        /* Greek Capital Letter Xi */
        CHAFA_SYMBOL_TAG_EXTRA,
        0x039e,
        "        "
        " XXXXXX "
        "        "
        "  XXXX  "
        "        "
        " XXXXXX "
        "        "
        "        "
    },
    {
        0, 0, ""
    }
};

/* ranges must be terminated by zero first, last */
static gboolean
unichar_is_in_ranges (gunichar c, const UnicharRange *ranges)
{
    for ( ; ranges->first != 0 || ranges->last != 0; ranges++)
    {
        g_assert (ranges->first <= ranges->last);

        if (c >= ranges->first && c <= ranges->last)
            return TRUE;
    }

    return FALSE;
}

static void
calc_weights (ChafaSymbol *sym)
{
    gint i;

    sym->fg_weight = 0;
    sym->bg_weight = 0;

    for (i = 0; i < CHAFA_SYMBOL_N_PIXELS; i++)
    {
        guchar p = sym->coverage [i];

        sym->fg_weight += p;
        sym->bg_weight += 1 - p;
    }
}

static void
xlate_coverage (const gchar *coverage_in, gchar *coverage_out)
{
    gchar xlate [256];
    gint i;

    xlate [' '] = 0;
    xlate ['X'] = 1;

    for (i = 0; i < CHAFA_SYMBOL_N_PIXELS; i++)
    {
        guchar p = (guchar) coverage_in [i];
        coverage_out [i] = xlate [p];
    }
}

static guint64
coverage_to_bitmap (const gchar *cov)
{
    guint64 bitmap = 0;
    gint i;

    for (i = 0; i < CHAFA_SYMBOL_N_PIXELS; i++)
    {
        bitmap <<= 1;

        if (cov [i])
            bitmap |= 1;
    }

    return bitmap;
}

static void
gen_braille_sym (gchar *cov, guint8 val)
{
    memset (cov, 0, CHAFA_SYMBOL_N_PIXELS);

    cov [1] = cov [2] = (val & 1);
    cov [5] = cov [6] = ((val >> 3) & 1);
    cov += CHAFA_SYMBOL_WIDTH_PIXELS * 2;

    cov [1] = cov [2] = ((val >> 1) & 1);
    cov [5] = cov [6] = ((val >> 4) & 1);
    cov += CHAFA_SYMBOL_WIDTH_PIXELS * 2;

    cov [1] = cov [2] = ((val >> 2) & 1);
    cov [5] = cov [6] = ((val >> 5) & 1);
    cov += CHAFA_SYMBOL_WIDTH_PIXELS * 2;

    cov [1] = cov [2] = ((val >> 6) & 1);
    cov [5] = cov [6] = ((val >> 7) & 1);
}

static void
generate_braille_syms (ChafaSymbol *syms, gint first_ofs)
{
    gunichar c;
    gint i = first_ofs;

    /* Braille 2x4 range */

    c = 0x2800;

    for (i = first_ofs; c < 0x2900; c++, i++)
    {
        ChafaSymbol *sym = &syms [i];

        sym->sc = CHAFA_SYMBOL_TAG_BRAILLE;
        sym->c = c;
        sym->coverage = g_malloc (CHAFA_SYMBOL_N_PIXELS);

        gen_braille_sym (sym->coverage, c - 0x2800);
        calc_weights (&syms [i]);
        syms [i].bitmap = coverage_to_bitmap (syms [i].coverage);
        syms [i].popcount = chafa_population_count_u64 (syms [i].bitmap);
    }
}

static ChafaSymbol *
init_symbol_array (const ChafaSymbolDef *defs)
{
    gint i;
    ChafaSymbol *syms;

    syms = g_new0 (ChafaSymbol, CHAFA_N_SYMBOLS_MAX);

    for (i = 0; defs [i].c; i++)
    {
        syms [i].sc = defs [i].sc;
        syms [i].c = defs [i].c;
        syms [i].coverage = g_malloc (CHAFA_SYMBOL_N_PIXELS);

        xlate_coverage (defs [i].coverage, syms [i].coverage);
        calc_weights (&syms [i]);
        syms [i].bitmap = coverage_to_bitmap (syms [i].coverage);
        syms [i].popcount = chafa_population_count_u64 (syms [i].bitmap);
    }

    generate_braille_syms (syms, i);
    return syms;
}

void
chafa_init_symbols (void)
{
    if (symbols_initialized)
        return;

    chafa_symbols = init_symbol_array (symbol_defs);

    symbols_initialized = TRUE;
}

ChafaSymbolTags
chafa_get_tags_for_char (gunichar c)
{
    ChafaSymbolTags tags = CHAFA_SYMBOL_TAG_NONE;
    gint i;

    chafa_init_symbols ();

    for (i = 0; chafa_symbols [i].c != 0; i++)
    {
        if (chafa_symbols [i].c == c)
            return chafa_symbols [i].sc | CHAFA_SYMBOL_TAG_NARROW;
    }

    if (g_unichar_iswide (c))
        tags |= CHAFA_SYMBOL_TAG_WIDE;
    else if (g_unichar_iswide_cjk (c))
        tags |= CHAFA_SYMBOL_TAG_AMBIGUOUS;

    if (g_unichar_ismark (c)
        || g_unichar_iszerowidth (c)
        || unichar_is_in_ranges (c, ambiguous_ranges))
        tags |= CHAFA_SYMBOL_TAG_AMBIGUOUS;

    if (unichar_is_in_ranges (c, emoji_ranges)
        || unichar_is_in_ranges (c, meta_ranges))
        tags |= CHAFA_SYMBOL_TAG_UGLY;

    if (c <= 0x7f)
        tags |= CHAFA_SYMBOL_TAG_ASCII;
    else if (c >= 0x2300 && c <= 0x23ff)
        tags |= CHAFA_SYMBOL_TAG_TECHNICAL;
    else if (c >= 0x25a0 && c <= 0x25ff)
        tags |= CHAFA_SYMBOL_TAG_GEOMETRIC;
    else if (c >= 0x2800 && c <= 0x28ff)
        tags |= CHAFA_SYMBOL_TAG_BRAILLE;
    else
        tags |= CHAFA_SYMBOL_TAG_EXTRA;

    if (g_unichar_isalpha (c))
        tags |= CHAFA_SYMBOL_TAG_ALPHA;
    if (g_unichar_isdigit (c))
        tags |= CHAFA_SYMBOL_TAG_DIGIT;

    if (!(tags & (CHAFA_SYMBOL_TAG_WIDE | CHAFA_SYMBOL_TAG_AMBIGUOUS)))
        tags |= CHAFA_SYMBOL_TAG_NARROW;

    return tags;
}
