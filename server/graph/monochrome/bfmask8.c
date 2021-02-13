/*
 * bfmask8.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- bitmasks for byte operations (bitfield emulation array)
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "bmono.h"


/*
 * BMONO driver needs this for couple of functions.
 * first index is the start bit and second the width.
 */

uchar	bfmask8[8][8] = {
  {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF},
  {0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E, 0x7F, 0x7F},
  {0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x3F},
  {0x10, 0x18, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F},
  {0x08, 0x0C, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F},
  {0x04, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07},
  {0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03},
  {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
};