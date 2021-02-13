/*
 * bfmask16.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- bitmasks for word (short int) operations
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "packed.h"


/*
 * the `packed' driver needs this for several routines.
 */

ushort	bfmask16[16][16] = {
{0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF},
{0x4000, 0x6000, 0x7000, 0x7800, 0x7C00, 0x7E00, 0x7F00, 0x7F80, 0x7FC0, 0x7FE0, 0x7FF0, 0x7FF8, 0x7FFC, 0x7FFE, 0x7FFF, 0x0000},
{0x2000, 0x3000, 0x3800, 0x3C00, 0x3E00, 0x3F00, 0x3F80, 0x3FC0, 0x3FE0, 0x3FF0, 0x3FF8, 0x3FFC, 0x3FFE, 0x3FFF, 0x0000, 0x0000},
{0x1000, 0x1800, 0x1C00, 0x1E00, 0x1F00, 0x1F80, 0x1FC0, 0x1FE0, 0x1FF0, 0x1FF8, 0x1FFC, 0x1FFE, 0x1FFF, 0x0000, 0x0000, 0x0000},
{0x0800, 0x0C00, 0x0E00, 0x0F00, 0x0F80, 0x0FC0, 0x0FE0, 0x0FF0, 0x0FF8, 0x0FFC, 0x0FFE, 0x0FFF, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0400, 0x0600, 0x0700, 0x0780, 0x07C0, 0x07E0, 0x07F0, 0x07F8, 0x07FC, 0x07FE, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0200, 0x0300, 0x0380, 0x03C0, 0x03E0, 0x03F0, 0x03F8, 0x03FC, 0x03FE, 0x03FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0100, 0x0180, 0x01C0, 0x01E0, 0x01F0, 0x01F8, 0x01FC, 0x01FE, 0x01FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0080, 0x00C0, 0x00E0, 0x00F0, 0x00F8, 0x00FC, 0x00FE, 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0040, 0x0060, 0x0070, 0x0078, 0x007C, 0x007E, 0x007F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0020, 0x0030, 0x0038, 0x003C, 0x003E, 0x003F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0010, 0x0018, 0x001C, 0x001E, 0x001F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0008, 0x000C, 0x000E, 0x000F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0004, 0x0006, 0x0007, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0002, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
{0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
};
