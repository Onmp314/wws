/*
 * copyright.h, part of Pente (game program)
 * Copyright (C) 1994-1995 William Shubert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * The author can be reached at wms@hevanet.com or wms@ssd.intel.com or
 *   William Shubert,
 *   1975 NW Everett St #301
 *   Portland OR 97209 USA.
 *   (503)223-2285
 *
 * Changes by Eero Tamminen (t150315@cc.tut.fi) for socket server version:
 * - Discarded all the wmslib and interface source.
 * - Added a couple of missing (no wmslib) defines to play.h.
 * - Removed all the string stuff needed for pipes (board.c, play.c).
 * - Removed all the wmslib 'magic' debug stuff from the rnd.[ch] files.
 * - Changed the generic include file from "../pente.h" to "play.h".
 * - Added glue code (engine.c) between mr. Schubert's Pente 'brain'
 *   and my GameFrame (networked, turn based, two player game framework)
 *   server code.
 *
 * The original source can be found from several Linux sites.
 */
