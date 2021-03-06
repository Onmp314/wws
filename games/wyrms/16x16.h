/* 16x8 array of 16x16 pixel bitmaps for Wyrms, includes the images for
 * game objects, numbers and letters.
 *
 * Copyright (C) 1996 by Eero Tamminen
 */

#define BLOCKS		127
#define BLK_W		16
#define BLK_H		16
#define BLK_ROW		16			/* blocks / row */
#define BLOCKS_W	(BLK_ROW * BLK_W)
#define BLOCKS_H	((BLOCKS + BLK_ROW - 1) / BLK_ROW * BLK_H)

static unsigned char Bitmaps[BLOCKS_W * BLOCKS_H / 8] =
{
  0x55, 0x55, 0x80, 0x01, 0x55, 0x3d, 0x7f, 0xf5, 
  0x4f, 0xf5, 0xff, 0x55, 0x55, 0x55, 0x55, 0x3d, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x5f, 0xf5, 0x00, 0x01, 0x00, 0x00, 
  0xaa, 0xaa, 0x40, 0x03, 0x80, 0x7e, 0xff, 0xfa, 
  0x9f, 0xfa, 0xc3, 0xaa, 0xaa, 0xaa, 0xaa, 0x7a, 
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x01, 0xbf, 0xfa, 0x00, 0x03, 0x00, 0x01, 
  0x55, 0x55, 0x20, 0x07, 0x07, 0xff, 0xf0, 0x1d, 
  0x7d, 0x3d, 0xa0, 0xd5, 0x54, 0x55, 0x54, 0xf5, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x7c, 0x3d, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x10, 0x0f, 0x9f, 0xff, 0xe0, 0x0e, 
  0xba, 0x9e, 0x98, 0x6a, 0xa9, 0xaa, 0xa9, 0xea, 
  0x0c, 0xc3, 0x01, 0x83, 0x1f, 0xfb, 0x00, 0x1b, 
  0x18, 0x03, 0xf0, 0xfe, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x08, 0x1f, 0x1f, 0xff, 0xc0, 0x0f, 
  0x7d, 0x1d, 0x84, 0x35, 0x53, 0xd5, 0x53, 0x95, 
  0x0c, 0xc3, 0x01, 0x83, 0x1f, 0xfb, 0x00, 0x7b, 
  0x1e, 0x03, 0xe0, 0x7f, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x04, 0x3f, 0xbf, 0xdf, 0xc0, 0x3f, 
  0xbe, 0x3e, 0x82, 0x1a, 0x87, 0xea, 0xa7, 0x00, 
  0x3f, 0xf3, 0x03, 0xc3, 0x0f, 0xf3, 0x01, 0xfb, 
  0x1f, 0x83, 0xe0, 0x7f, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x02, 0x7f, 0x3f, 0xcf, 0xc0, 0x0f, 
  0x5f, 0xfd, 0x41, 0x1d, 0x1f, 0xf5, 0x4f, 0xff, 
  0x3f, 0xf3, 0x03, 0xc3, 0x0f, 0xf3, 0x07, 0xfb, 
  0x1f, 0xe3, 0xc0, 0x7f, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x00, 0xff, 0x3f, 0xcf, 0xc8, 0x0f, 
  0xaf, 0xfa, 0xc0, 0x8a, 0x3f, 0xfa, 0x9f, 0xfe, 
  0x0c, 0xc3, 0x07, 0xe3, 0x07, 0xe3, 0x1f, 0xfb, 
  0x1f, 0xfb, 0xc0, 0x7f, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x01, 0x7f, 0xff, 0x8f, 0xc8, 0x3f, 
  0x57, 0xd5, 0x60, 0x8d, 0x7f, 0xfd, 0x3f, 0xfd, 
  0x0c, 0xc3, 0x07, 0xe3, 0x07, 0xe3, 0x1f, 0xfb, 
  0x1f, 0xfb, 0xc0, 0xf7, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x03, 0xbf, 0xff, 0x0f, 0xc8, 0x0f, 
  0xab, 0xc2, 0xb0, 0x4a, 0x7f, 0xfe, 0xff, 0xfa, 
  0x3f, 0xf3, 0x0f, 0xf3, 0x03, 0xc3, 0x07, 0xfb, 
  0x1f, 0xe3, 0xd3, 0xe7, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x07, 0xdf, 0xf8, 0x1f, 0xc0, 0x0f, 
  0x57, 0xfd, 0x58, 0x2d, 0xff, 0xff, 0x54, 0xf5, 
  0x3f, 0xf3, 0x0f, 0xf3, 0x03, 0xc3, 0x01, 0xfb, 
  0x1f, 0x83, 0xdf, 0xcf, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x0f, 0xef, 0xfc, 0x3f, 0xc0, 0x3f, 
  0xab, 0xfe, 0xac, 0x2a, 0xff, 0xfe, 0xa9, 0xea, 
  0x0c, 0xc3, 0x1f, 0xfb, 0x01, 0x83, 0x00, 0x7b, 
  0x1e, 0x03, 0xff, 0x9f, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x1f, 0xf7, 0xff, 0xff, 0xc0, 0x0f, 
  0x57, 0xd5, 0x57, 0xfd, 0x55, 0x55, 0x53, 0xd5, 
  0x0c, 0xc3, 0x1f, 0xfb, 0x01, 0x83, 0x00, 0x1b, 
  0x18, 0x03, 0x7f, 0x7f, 0x3f, 0xff, 0x00, 0x03, 
  0xaa, 0xaa, 0x3f, 0xfb, 0xff, 0xfe, 0xc0, 0x0f, 
  0xab, 0xc2, 0xaa, 0xbe, 0xaa, 0xaa, 0xa7, 0xaa, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0xbf, 0xfe, 0x3f, 0xff, 0x00, 0x03, 
  0x55, 0x55, 0x7f, 0xfd, 0x7f, 0xfd, 0xff, 0xff, 
  0x57, 0xfd, 0x55, 0x5d, 0x55, 0x55, 0x4f, 0x55, 
  0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 
  0x3f, 0xff, 0x5f, 0xfd, 0x7f, 0xff, 0x3f, 0xff, 
  0xaa, 0xaa, 0xff, 0xff, 0xbf, 0xfa, 0xff, 0xff, 
  0xab, 0xfe, 0xaa, 0xbe, 0xaa, 0xaa, 0x9c, 0xaa, 
  0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 
  0x7f, 0xff, 0xaf, 0xfa, 0xff, 0xff, 0x7f, 0xff, 
  0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0x00, 0x01, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x3f, 0x03, 0x00, 0xff, 0x3f, 0xf3, 
  0x07, 0x83, 0x03, 0x03, 0x0f, 0xc3, 0x1f, 0xe3, 
  0x03, 0xc3, 0x1f, 0xf3, 0x0f, 0xc3, 0x3f, 0xf3, 
  0x0f, 0xc3, 0x0f, 0xc3, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x3f, 0x03, 0x00, 0xff, 0x3f, 0xf3, 
  0x1f, 0xe3, 0x07, 0x03, 0x1f, 0xe3, 0x1f, 0xe3, 
  0x07, 0xc3, 0x1f, 0xf3, 0x1f, 0xe3, 0x3f, 0xf3, 
  0x1f, 0xe3, 0x1f, 0xe3, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x3f, 0x03, 0x00, 0xff, 0x30, 0x03, 
  0x1c, 0xe3, 0x0f, 0x03, 0x38, 0x73, 0x11, 0x83, 
  0x0e, 0xc3, 0x18, 0x03, 0x38, 0x23, 0x30, 0x33, 
  0x30, 0x33, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x3f, 0x03, 0x00, 0xff, 0x30, 0x03, 
  0x38, 0x73, 0x1f, 0x03, 0x10, 0x73, 0x03, 0x83, 
  0x1c, 0xc3, 0x18, 0x03, 0x30, 0x03, 0x20, 0x73, 
  0x30, 0x33, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x3f, 0x03, 0x00, 0xff, 0x30, 0x33, 
  0x38, 0x73, 0x03, 0x03, 0x01, 0xe3, 0x01, 0xc3, 
  0x38, 0xc3, 0x1f, 0xc3, 0x3f, 0xc3, 0x00, 0xe3, 
  0x1f, 0xe3, 0x1f, 0xf3, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x3f, 0x03, 0x00, 0xff, 0x30, 0x33, 
  0x38, 0x73, 0x03, 0x03, 0x07, 0xc3, 0x08, 0xe3, 
  0x30, 0xc3, 0x1f, 0xe3, 0x3f, 0xe3, 0x01, 0xc3, 
  0x1f, 0xe3, 0x0f, 0xf3, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x00, 0xff, 0x3f, 0x03, 0x30, 0x33, 
  0x38, 0x73, 0x03, 0x03, 0x0f, 0x03, 0x18, 0x63, 
  0x3f, 0xf3, 0x00, 0x73, 0x30, 0x73, 0x03, 0x83, 
  0x30, 0x33, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x00, 0xff, 0x3f, 0x03, 0x30, 0x33, 
  0x1c, 0xe3, 0x03, 0x03, 0x1c, 0x03, 0x18, 0xe3, 
  0x3f, 0xf3, 0x00, 0x73, 0x38, 0x73, 0x03, 0x03, 
  0x30, 0x33, 0x10, 0x73, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x00, 0xff, 0x3f, 0x03, 0x33, 0xf3, 
  0x1f, 0xe3, 0x0f, 0xc3, 0x3f, 0xf3, 0x1f, 0xe3, 
  0x00, 0xc3, 0x1f, 0xe3, 0x1f, 0xe3, 0x03, 0x03, 
  0x1f, 0xe3, 0x1f, 0xe3, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x00, 0xff, 0x3f, 0x03, 0x33, 0xf3, 
  0x07, 0x83, 0x0f, 0xc3, 0x3f, 0xf3, 0x0f, 0xc3, 
  0x00, 0xc3, 0x0f, 0xc3, 0x0f, 0xc3, 0x03, 0x03, 
  0x0f, 0xc3, 0x0f, 0xc3, 0x00, 0x00, 0x00, 0x00, 
  0x15, 0x57, 0x00, 0xff, 0x3f, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x2a, 0xab, 0x00, 0xff, 0x3f, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x7f, 0xff, 0x3f, 0xff, 0x7f, 0xff, 0xff, 0xff, 
  0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 
  0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 
  0x7f, 0xff, 0x7f, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x0f, 0xfc, 0x3c, 0x3c, 0x3c, 0xf0, 0x03, 0xc0, 
  0x00, 0x3c, 0x3c, 0x00, 0x3c, 0x3c, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0xf0, 0x03, 0xc0, 
  0x00, 0xf0, 0x0f, 0x00, 0x3c, 0x3c, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0xff, 0xff, 
  0x3c, 0x00, 0x3c, 0xf0, 0x0f, 0xc0, 0x03, 0xc0, 
  0x03, 0xf0, 0x0f, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0xff, 0xff, 
  0x3c, 0x00, 0x00, 0xf0, 0x0f, 0xc0, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3f, 0xf0, 0x03, 0xc0, 0x3f, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0xff, 0xff, 0x3f, 0xfc, 
  0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0xf0, 
  0x00, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x0f, 0xfc, 0x03, 0xc0, 0x3f, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0xff, 0xff, 0x3f, 0xfc, 
  0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x03, 0xc0, 
  0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0xff, 0xff, 
  0x00, 0x3c, 0x0f, 0x00, 0xf3, 0xfc, 0x00, 0x00, 
  0x03, 0xc0, 0x03, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0xff, 0xff, 
  0x00, 0x3c, 0x0f, 0x3c, 0xf3, 0xfc, 0x00, 0x00, 
  0x03, 0xc0, 0x03, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x3c, 
  0x3f, 0xfc, 0x3c, 0x3c, 0xf0, 0xf0, 0x00, 0x00, 
  0x03, 0xc0, 0x03, 0xc0, 0x3c, 0x3c, 0x03, 0xc0, 
  0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x0f, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x3c, 
  0x3f, 0xf0, 0x3c, 0x3c, 0xf0, 0xf0, 0x00, 0x00, 
  0x03, 0xf0, 0x0f, 0xc0, 0x3c, 0x3c, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x3c, 0x00, 
  0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 
  0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x3c, 0x00, 
  0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x3f, 0x3c, 0x00, 0x00, 
  0x00, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x3c, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0xf0, 0x03, 0xc0, 0x0f, 0xf0, 0x3f, 0xfc, 
  0x00, 0xf0, 0x3f, 0xfc, 0x03, 0xf0, 0x3f, 0xfc, 
  0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 
  0x3f, 0xfc, 0x03, 0xc0, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x00, 0xf0, 0x3f, 0xfc, 0x0f, 0xf0, 0x3f, 0xfc, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xfc, 0x00, 0x00, 0xfc, 0x00, 0x3f, 0xfc, 
  0x3c, 0x3c, 0x0f, 0xc0, 0x3c, 0x3c, 0x00, 0xf0, 
  0x03, 0xf0, 0x3c, 0x00, 0x3f, 0x00, 0x00, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x03, 0xc0, 0x03, 0xc0, 
  0x03, 0xf0, 0x3f, 0xfc, 0x3f, 0x00, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x0f, 0xc0, 0x3c, 0x3c, 0x00, 0xf0, 
  0x03, 0xf0, 0x3c, 0x00, 0x3c, 0x00, 0x00, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x03, 0xc0, 0x03, 0xc0, 
  0x0f, 0xc0, 0x3f, 0xfc, 0x0f, 0xc0, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0x03, 0xc0, 
  0x0f, 0xf0, 0x3f, 0xf0, 0x3c, 0x00, 0x00, 0xf0, 
  0x0f, 0xf0, 0x3f, 0xfc, 0x03, 0xc0, 0x03, 0xc0, 
  0x3f, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0xf0, 
  0x3c, 0xfc, 0x03, 0xc0, 0x00, 0xf0, 0x03, 0xc0, 
  0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xf0, 0x00, 0xf0, 
  0x0f, 0xf0, 0x0f, 0xfc, 0x03, 0xc0, 0x03, 0xc0, 
  0xfc, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xf0, 
  0x3f, 0x3c, 0x03, 0xc0, 0x03, 0xc0, 0x00, 0xf0, 
  0x3c, 0xf0, 0x00, 0x3c, 0x3f, 0xfc, 0x03, 0xc0, 
  0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0x00, 0x3f, 0xfc, 0x03, 0xf0, 0x03, 0xc0, 
  0x3c, 0x3c, 0x03, 0xc0, 0x03, 0xc0, 0x00, 0xf0, 
  0x3c, 0xf0, 0x00, 0x3c, 0x3c, 0x3c, 0x03, 0xc0, 
  0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0xc0, 0x3f, 0xfc, 0x0f, 0xc0, 0x03, 0xc0, 
  0x3c, 0x3c, 0x03, 0xc0, 0x0f, 0x00, 0x3c, 0x3c, 
  0x3f, 0xfc, 0x00, 0x3c, 0x3c, 0x3c, 0x0f, 0x00, 
  0x3c, 0x3c, 0x00, 0x3c, 0x03, 0xc0, 0x03, 0xc0, 
  0x03, 0xf0, 0x00, 0x00, 0x3f, 0x00, 0x03, 0xc0, 
  0x3c, 0x3c, 0x03, 0xc0, 0x0f, 0x00, 0x3c, 0x3c, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x0f, 0x00, 
  0x3c, 0x3c, 0x00, 0xfc, 0x03, 0xc0, 0x03, 0xc0, 
  0x00, 0xfc, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x00, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0x00, 
  0x3f, 0xfc, 0x0f, 0xf0, 0x03, 0xc0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 
  0x00, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0x00, 
  0x0f, 0xf0, 0x0f, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0xc0, 0x03, 0xc0, 0x3f, 0xf0, 0x0f, 0xf0, 
  0x3f, 0xc0, 0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xfc, 
  0x3c, 0x3c, 0x3f, 0xfc, 0x00, 0x3c, 0xf0, 0xf0, 
  0x3c, 0x00, 0xf0, 0x3c, 0x3c, 0x3c, 0x0f, 0xf0, 
  0x3f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3c, 0x3c, 0x3f, 0xfc, 0x00, 0x3c, 0xf0, 0xf0, 
  0x3c, 0x00, 0xf0, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 
  0xfc, 0x3c, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3c, 0xfc, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0x3c, 0xf3, 0xc0, 
  0x3c, 0x00, 0xfc, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 
  0xf0, 0x0c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0x3c, 0xf3, 0xc0, 
  0x3c, 0x00, 0xfc, 0xfc, 0x3f, 0x3c, 0x3c, 0x3c, 
  0xf3, 0xcc, 0x3c, 0x3c, 0x3f, 0xfc, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3f, 0xf0, 0x3f, 0xf0, 0x3c, 0xfc, 
  0x3f, 0xfc, 0x03, 0xc0, 0x00, 0x3c, 0xff, 0x00, 
  0x3c, 0x00, 0xff, 0xfc, 0x3f, 0x3c, 0x3c, 0x3c, 
  0xf3, 0x3c, 0x3c, 0x3c, 0x3f, 0xf0, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3f, 0xf0, 0x3f, 0xf0, 0x3c, 0xfc, 
  0x3f, 0xfc, 0x03, 0xc0, 0x00, 0x3c, 0xff, 0x00, 
  0x3c, 0x00, 0xf3, 0x3c, 0x3f, 0xfc, 0x3c, 0x3c, 
  0xf3, 0x3c, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0x3c, 0xf3, 0xc0, 
  0x3c, 0x00, 0xf3, 0x3c, 0x3f, 0xfc, 0x3c, 0x3c, 
  0xf3, 0xf0, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0x3c, 0xf3, 0xc0, 
  0x3c, 0x00, 0xf0, 0x3c, 0x3c, 0xfc, 0x3c, 0x3c, 
  0xf0, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x3c, 0x3c, 0xf0, 0xf0, 
  0x3c, 0x00, 0xf0, 0x3c, 0x3c, 0xfc, 0x3c, 0x3c, 
  0xfc, 0x0c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3c, 0xfc, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x3c, 0x3c, 0xf0, 0xf0, 
  0x3c, 0x00, 0xf0, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3f, 0xf0, 0x3f, 0xfc, 0x3c, 0x00, 0x3f, 0xfc, 
  0x3c, 0x3c, 0x3f, 0xfc, 0x3f, 0xfc, 0xf0, 0x3c, 
  0x3f, 0xfc, 0xf0, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 
  0x0f, 0xf0, 0x3c, 0x3c, 0x3f, 0xf0, 0x0f, 0xf0, 
  0x3f, 0xc0, 0x3f, 0xfc, 0x3c, 0x00, 0x0f, 0xf0, 
  0x3c, 0x3c, 0x3f, 0xfc, 0x0f, 0xf0, 0xf0, 0x3c, 
  0x3f, 0xfc, 0xf0, 0x3c, 0x3c, 0x3c, 0x0f, 0xf0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
  0x3f, 0xf0, 0x0f, 0xf0, 0xff, 0xc0, 0x0f, 0xfc, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 0x03, 0xfc, 
  0x3c, 0x00, 0x3f, 0xc0, 0x03, 0x00, 0x00, 0x00, 
  0x3f, 0xfc, 0x3f, 0xfc, 0xff, 0xf0, 0x3f, 0xfc, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 0x03, 0xfc, 
  0x3c, 0x00, 0x3f, 0xc0, 0x0f, 0xc0, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0xf0, 0x3c, 0x00, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0xf0, 0x03, 0xc0, 
  0x3c, 0x00, 0x03, 0xc0, 0x0f, 0xc0, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0xf0, 0x3c, 0x00, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x0f, 0xf0, 0x3c, 0x3c, 0x00, 0xf0, 0x03, 0xc0, 
  0x0f, 0x00, 0x03, 0xc0, 0x3c, 0xf0, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0xf0, 0x3f, 0x00, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x0f, 0xf0, 0x0f, 0xf0, 0x03, 0xc0, 0x03, 0xc0, 
  0x0f, 0x00, 0x03, 0xc0, 0x3c, 0xf0, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0xff, 0xf0, 0x0f, 0xc0, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf3, 0x3c, 
  0x03, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0xf0, 0x3c, 0x00, 0x00, 
  0x3f, 0xfc, 0x3c, 0x3c, 0xff, 0xc0, 0x03, 0xf0, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf3, 0x3c, 
  0x03, 0xc0, 0x03, 0xc0, 0x0f, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x03, 0xc0, 0xf0, 0x3c, 0x00, 0x00, 
  0x3f, 0xf0, 0x3c, 0x3c, 0xf3, 0xc0, 0x00, 0xfc, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xff, 0xfc, 
  0x0f, 0xf0, 0x03, 0xc0, 0x0f, 0x00, 0x03, 0xc0, 
  0x00, 0xf0, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x3c, 0x3c, 0xf0, 0xf0, 0x00, 0x3c, 
  0x03, 0xc0, 0x3c, 0x3c, 0x0f, 0xf0, 0xff, 0xfc, 
  0x0f, 0xf0, 0x03, 0xc0, 0x3c, 0x00, 0x03, 0xc0, 
  0x00, 0xf0, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x3c, 0xcc, 0xf0, 0xf0, 0x00, 0x3c, 
  0x03, 0xc0, 0x3c, 0x3c, 0x0f, 0xf0, 0xfc, 0xfc, 
  0x3c, 0x3c, 0x03, 0xc0, 0x3c, 0x00, 0x03, 0xc0, 
  0x00, 0x3c, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x3f, 0xf0, 0xf0, 0x3c, 0x3f, 0xfc, 
  0x03, 0xc0, 0x3f, 0xfc, 0x03, 0xc0, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x3f, 0xfc, 0x03, 0xfc, 
  0x00, 0x3c, 0x3f, 0xc0, 0x00, 0x00, 0xff, 0xfc, 
  0x3c, 0x00, 0x0f, 0x3c, 0xf0, 0x3c, 0x3f, 0xf0, 
  0x03, 0xc0, 0x0f, 0xf0, 0x03, 0xc0, 0xc0, 0x0c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x3f, 0xfc, 0x03, 0xfc, 
  0x00, 0x3c, 0x3f, 0xc0, 0x00, 0x00, 0xff, 0xfc, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 
  0x3c, 0x00, 0x03, 0xc0, 0x00, 0xf0, 0xf0, 0x00, 
  0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x0f, 0xc0, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 
  0x3c, 0x00, 0x03, 0xc0, 0x00, 0xf0, 0xf0, 0x00, 
  0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xf0, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 
  0x00, 0x3c, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 
  0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xf0, 0x0f, 0xf0, 0x3f, 0xf0, 0x0f, 0xf0, 
  0x0f, 0xfc, 0x0f, 0xf0, 0x03, 0xc0, 0x0f, 0xfc, 
  0x3f, 0xf0, 0x0f, 0xc0, 0x00, 0xf0, 0xf0, 0xf0, 
  0x03, 0xc0, 0x3c, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 
  0x00, 0x30, 0x0f, 0xfc, 0x3f, 0xfc, 0x3f, 0xf0, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3f, 0xfc, 0x0f, 0xc0, 0x00, 0xf0, 0xf3, 0xf0, 
  0x03, 0xc0, 0xff, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0xff, 0xc0, 
  0x03, 0xc0, 0xff, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x00, 0x00, 0x0f, 0xfc, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x03, 0xc0, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0xff, 0x00, 
  0x03, 0xc0, 0xf3, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x00, 0x00, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3f, 0xfc, 0x03, 0xc0, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0xff, 0xc0, 
  0x03, 0xc0, 0xf3, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x00, 0x03, 0xc0, 0x3c, 0x3c, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0xf3, 0xc0, 
  0x03, 0xc0, 0xf3, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 
  0x3c, 0x3c, 0x3c, 0x00, 0x03, 0xc0, 0x3f, 0xfc, 
  0x3c, 0x3c, 0x03, 0xc0, 0x00, 0xf0, 0xf0, 0xf0, 
  0x03, 0xc0, 0xf0, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 
  0x00, 0x00, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x03, 0xc0, 0x0f, 0xfc, 
  0x3c, 0x3c, 0x0f, 0xf0, 0x00, 0xf0, 0xf0, 0xfc, 
  0x0f, 0xf0, 0xf0, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 
  0x00, 0x00, 0x0f, 0xfc, 0x3f, 0xf0, 0x0f, 0xfc, 
  0x0f, 0xfc, 0x0f, 0xfc, 0x03, 0xc0, 0x00, 0x3c, 
  0x3c, 0x3c, 0x0f, 0xf0, 0x00, 0xf0, 0xf0, 0x3c, 
  0x0f, 0xf0, 0xf0, 0x3c, 0x3c, 0x3c, 0x0f, 0xf0, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 
  0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 
  0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 
  0x03, 0xc0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0xf0, 0x0f, 0xfc, 0x3f, 0xf0, 0x0f, 0xfc, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x3c, 0x0c, 0x00, 0x00, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 
  0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3f, 0xfc, 0x0f, 0xc0, 
  0x03, 0xc0, 0x0f, 0xc0, 0xff, 0x0c, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf3, 0x3c, 
  0x0f, 0xf0, 0x3c, 0x3c, 0x00, 0xf0, 0xff, 0x00, 
  0x03, 0xc0, 0x03, 0xfc, 0xcf, 0xfc, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x3f, 0x00, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xf3, 0x3c, 
  0x0f, 0xf0, 0x3c, 0x3c, 0x03, 0xc0, 0xff, 0x00, 
  0x03, 0xc0, 0x03, 0xfc, 0xc3, 0xf0, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x0f, 0xf0, 
  0x03, 0xc0, 0x3c, 0x3c, 0x3c, 0x3c, 0xff, 0xfc, 
  0x03, 0xc0, 0x3c, 0x3c, 0x03, 0xc0, 0x0f, 0xc0, 
  0x03, 0xc0, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0xfc, 
  0x03, 0xc0, 0x3c, 0x3c, 0x0f, 0xf0, 0xff, 0xfc, 
  0x0f, 0xf0, 0x3c, 0x3c, 0x0f, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0x3c, 
  0x03, 0xc0, 0x3c, 0x3c, 0x0f, 0xf0, 0xfc, 0xfc, 
  0x0f, 0xf0, 0x3f, 0xfc, 0x0f, 0x00, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x00, 0x3f, 0xfc, 
  0x03, 0xfc, 0x3f, 0xfc, 0x03, 0xc0, 0xf0, 0x3c, 
  0x3c, 0x3c, 0x0f, 0xfc, 0x3f, 0xfc, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0xf0, 0x0f, 0xfc, 0x3c, 0x00, 0x3f, 0xf0, 
  0x00, 0xfc, 0x0f, 0xfc, 0x03, 0xc0, 0xc0, 0x0c, 
  0x3c, 0x3c, 0x00, 0x3c, 0x3f, 0xfc, 0x03, 0xc0, 
  0x03, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0xfc, 
  0x03, 0xc0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
