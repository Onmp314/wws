/*
 * fontconv.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- convert X font snapshot to a fixed width W font
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/Wlib.h"

#define MEDBUF 256
#define SMALLBUF 80
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define ushort unsigned short


typedef struct {
  int width, height;
  int bpl;
  void *data;
} PBMP4;


typedef struct {
  long magic;
  ushort flags;
  short height;
  char widths[256];
} FONTHDR;


/*
 *
 */

static PBMP4 *allocPbmP4(int width, int height)
{
  PBMP4 *ret;
  int size;

  if (!(ret = malloc(sizeof(PBMP4)))) {
    fprintf(stderr, "out of memory\n");
    return NULL;
  }

  ret->width = width;
  ret->height = height;
  ret->bpl = ((width + 7) & ~7) >> 3;
  size = ret->bpl * height;

  if (!(ret->data = malloc(size))) {
    fprintf(stderr, "out of memory\n");
    free(ret);
    return NULL;
  }

  memset(ret->data, 0, size);

  return ret;
}


static void freePbmP4(PBMP4 *pic)
{
  free(pic->data);
  free(pic);
}


static PBMP4 *loadPbmP4(char *fname)
{
  FILE *fp;
  char buf[MEDBUF];
  int line, width, height, size;
  PBMP4 *ret;

  printf("%s: ", fname);
  fflush(stdout);

  if (!(fp = fopen(fname, "r"))) {
    fprintf(stderr, "can't open file\n");
    return NULL;
  }

  line = 0;
  while (line < 2) {
    fgets(buf, MEDBUF, fp);
    if (*buf != '#') {
      line++;
      if (line == 1) {
	if (strcmp(buf, "P4\n")) {
	  fprintf(stderr, "is not a PBM-P4 file\n");
	  fclose(fp);
	  return NULL;
	}
      } else {
	sscanf(buf, "%i %i", &width, &height);
      }
    }
  }

  if (!(ret = allocPbmP4(width, height))) {
    fclose(fp);
    return NULL;
  }

  size = ret->bpl * height;
  if (fread(ret->data, 1, size, fp) != size) {
    fprintf(stderr, "can't read file\n");
    free(ret->data);
    free(ret);
    fclose(fp);
    return NULL;
  }

  printf("is a PBM-P4 file with %ix%i pixel\n", width, height);
  fclose(fp);

  return ret;
}


static void savePbmP4(PBMP4 *pic, char *fname)
{
  FILE *fp;

  if (!(fp = fopen(fname, "w"))) {
    return;
  }

  fprintf(fp, "P4\n");
  fprintf(fp, "%i %i\n", pic->width, pic->height);
  fwrite(pic->data, 1, pic->bpl * pic->height, fp);
  fclose(fp);
}


static inline int
test(register PBMP4 *pic, register int x0, register int y0)
{
  return (*((unsigned char *)pic->data + pic->bpl * y0 + (x0 >> 3))) &
    (128 >> (x0 & 7));
}


static inline void
set(register PBMP4 *pic, register int x0, register int y0)
{
  *((unsigned char *)pic->data + pic->bpl * y0 + (x0 >> 3)) |=
    128 >> (x0 & 7);
}


static inline void
invert(register PBMP4 *pic, register int x0, register int y0)
{
  *((unsigned char *)pic->data + pic->bpl * y0 + (x0 >> 3)) ^=
    128 >> (x0 & 7);
}


static inline void
clear(register PBMP4 *pic, register int x0, register int y0)
{
  *((unsigned char *)pic->data + pic->bpl * y0 + (x0 >> 3)) &=
    ~(128 >> (x0 & 7));
}


static int checkHline(PBMP4 *pic, int y)
{
  int x = 0, xe = pic->width - 1;

  while (x <= xe) {
    if (test(pic, x, y)) {
      return 1;
    }
    x++;
  }

  return 0;
}


static int checkVline(PBMP4 *pic, int x, int y0, int ye)
{
  while (y0 <= ye) {
    if (test(pic, x, y0)) {
      return 1;
    }
    y0++;
  }

  return 0;
}


static void findFrame(PBMP4 *pic, int *fx0, int *fy0, int *fwidth, int *fheight)
{
  int x0, y0, xe, ye;

  *fx0 = *fy0 = 0;
  *fwidth = *fheight = 0;

  x0 = 0;
  xe = pic->width - 1;

  /* lower line */

  ye = pic->height;
  while (ye > 0) {
    ye--;
    if (checkHline(pic, ye)) {
      break;
    }
  }

  /* upper line */

  y0 = ye;
  while (y0 > 0) {
    y0--;
    if (!checkHline(pic, y0)) {
      y0++;
      break;
    }
  }

  *fy0 = y0;
  *fheight = ye - y0 + 1;

  /* right line */

  xe = pic->width;
  while (xe > 0) {
    xe--;
    if (checkVline(pic, xe, y0, ye)) {
      break;
    }
  }

  /* left line */

  x0 = xe;
  while (x0 >= 0) {
    x0--;
    if (!checkVline(pic, x0, y0, ye)) {
      x0++;
      break;
    }
  }

  *fx0 = x0;
  *fwidth = xe - x0 + 1;

  printf("\tframe is at %i,%i with size %i,%i\n",
	 *fx0, *fy0, *fwidth, *fheight);
}


static void scanBox(PBMP4 *pic,
	     int bx0, int by0, int bwidth, int bheight,
	     int *cx0, int *cy0, int *cxe, int *cye)
{
  int x, y, tx0, ty0, txe, tye;

  tx0 = bwidth - 1;
  ty0 = bheight - 1;
  txe = tye = 0;
  for (y=0; y<bheight; y++) {
    for (x=0; x<bwidth; x++) {
      if (test(pic, bx0 + x, by0 + y)) {
	tx0 = MIN(tx0, x);
	ty0 = MIN(ty0, y);
	txe = MAX(txe, x);
	tye = MAX(tye, y);
      }
    }
  }

  *cx0 = tx0;
  *cy0 = ty0;
  *cxe = txe;
  *cye = tye;
}


static void scanFrame(PBMP4 *pic, int fx0, int fy0, int fwidth, int fheight,
	       int *cx0, int *cy0, int *cwidth, int *cheight)
{
  int bwidth, bheight, bxoffset, byoffset, x, y;
  int mcx0, mcy0, mcxe, mcye, tcx0, tcy0, tcxe, tcye;

  printf("\tscanning frame... ");
  fflush(stdout);

  bxoffset = fwidth >> 4;
  byoffset = fheight >> 4;
  bwidth = bxoffset - 1;
  bheight = byoffset - 1;

  mcx0 = bwidth - 1;
  mcy0 = bheight - 1;
  mcxe = 0;
  mcye = 0;

  for (y=0; y<16; y++) {
    for (x=0; x<16; x++) {
      scanBox(pic,
	      fx0 + x * bxoffset, fy0 + y * byoffset, bwidth, bheight,
	      &tcx0, &tcy0, &tcxe, &tcye);
      mcx0 = MIN(mcx0, tcx0);
      mcy0 = MIN(mcy0, tcy0);
      mcxe = MAX(mcxe, tcxe);
      mcye = MAX(mcye, tcye);
    }
  }

  printf("biggest char is %ix%i at %i,%i\n",
	 mcxe - mcx0 + 1, mcye - mcy0 + 1, mcx0, mcy0);

  *cx0 = mcx0;
  *cy0 = mcy0;
  *cwidth = mcxe - mcx0 + 1;
  *cheight = mcye - mcy0 + 1;
}


static void copyChar2(PBMP4 *pic, int x0, int y0, int width, int height,
	       PBMP4 *out, int x1, int y1)
{
  int x, y;

  for (y=0; y<height; y++) {
    for (x=0; x<width; x++) {
      if (test(pic, x0 + x, y0 + y)) {
	set(out, x1 + x, y1 + y);
      }
    }
  }
}


static void extractChars2(char *fname,
		  PBMP4 *pic, int fx0, int fy0, int fwidth, int fheight,
		  int cx0, int cy0, int cwidth, int cheight)
{
  char oname[SMALLBUF], *cptr;
  PBMP4 *out;
  int bwidth, bheight, bxoffset, byoffset, x, y;

  printf("\textracting chars... ");
  fflush(stdout);

  strcpy(oname, fname);
  if ((cptr = rindex(oname, '.'))) {
    *cptr = 0;
  }
  strcat(oname, ".wfnt");

  if (!(out = allocPbmP4(cwidth << 4, cheight << 4))) {
    return;
  }

  bxoffset = fwidth >> 4;
  byoffset = fheight >> 4;
  bwidth = bxoffset - 1;
  bheight = byoffset - 1;

  for (y=0; y<16; y++) {
    for (x=0; x<16; x++) {
      copyChar2(pic, fx0 + x * bxoffset + cx0, fy0 + y * byoffset + cy0,
		cwidth, cheight,
		out, cwidth * x, cheight * y);
    }
  }

  savePbmP4(out, oname);

  freePbmP4(out);

  printf("done, font written to: %s\n", oname);
}


static inline void fputl(long l, FILE *fp)
{
  fwrite(&l, 1, 4, fp);
}


static void copyChar(PBMP4 *pic, int x0, int y0, int width, int height, FILE *fp)
{
  int x, y;
  unsigned long val, bit;

  val = 0;
  bit = 0x80000000;
  for (y=0; y<height; y++) {
    for (x=0; x<width; x++) {
      if (test(pic, x0 + x, y0 + y)) {
	val |= bit;
      }
      if (!(bit >>= 1)) {
	bit = 0x80000000;
	fputl(val, fp);
	val = 0;
      }
    }
  }
  if (bit != 0x80000000) {
    fputl(val, fp);
  }
}


static void extractChars(char *fname,
		  PBMP4 *pic, int fx0, int fy0, int fwidth, int fheight,
		  int cx0, int cy0, int cwidth, int cheight)
{
  char oname[SMALLBUF], *cptr, buf[SMALLBUF];
  int bwidth, bheight, bxoffset, byoffset, x, y, i;
  FONTHDR fonthdr;
  FILE *fp;

  fonthdr.magic = 0x57464E54;
  fonthdr.flags = 0;
  fonthdr.height = cheight;
  for (i=0; i<256; i++) {
    fonthdr.widths[i] = cwidth;
  }

  /* manually edit flags field */
  printf("\tedit flags ('b', 'i') : ");
  fflush(stdout);
  if ((cptr = fgets(buf, SMALLBUF, stdin))) {
    while (*cptr) {
      switch (*cptr) {
        case 'b':
	  fonthdr.flags |= F_BOLD;
	  break;
        case 'i':
	  fonthdr.flags |= F_ITALIC;
	  break;
      }
      cptr++;
    }
  }

  printf("\textracting chars... ");
  fflush(stdout);

  strcpy(oname, fname);
  if ((cptr = rindex(oname, '.'))) {
    *cptr = 0;
  }
  strcat(oname, ".wfnt");

  bxoffset = fwidth >> 4;
  byoffset = fheight >> 4;
  bwidth = bxoffset - 1;
  bheight = byoffset - 1;

  if (!(fp = fopen(oname, "w"))) {
    fprintf(stderr, "can't create %s\n", oname);
    return;
  }

  fwrite(&fonthdr, 1, sizeof(FONTHDR), fp);

  for (y=0; y<16; y++) {
    for (x=0; x<16; x++) {
      copyChar(pic, fx0 + x * bxoffset + cx0, fy0 + y * byoffset + cy0,
	       cwidth, cheight, fp);
    }
  }

  fclose(fp);

  printf("done, font written to: %s\n", oname);
}


static void conv(char *fname)
{
  PBMP4 *pic;
  int fx0, fy0, fwidth, fheight, cx0, cy0, cwidth, cheight;

  if (!(pic = loadPbmP4(fname))) {
    return;
  }

  findFrame(pic, &fx0, &fy0, &fwidth, &fheight);

  fx0++;
  fy0++;
  fwidth--;
  fheight--;

  scanFrame(pic, fx0, fy0, fwidth, fheight,
	    &cx0, &cy0, &cwidth, &cheight);

  extractChars(fname,
	       pic, fx0, fy0, fwidth, fheight,
	       cx0, cy0, cwidth, cheight);

  freePbmP4(pic);
}


/*
 * guess what...
 */

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2) {
    fprintf(stderr, "usage: %s {<xfd-shots>.pbm}\n", argv[0]);
    return -1;
  }

  for (i=1; i<argc; i++) {
    conv(argv[i]);
  }

  return 0;
}
