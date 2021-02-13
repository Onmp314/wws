/*
 * img.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1996-1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines to read a monochrome IMG file as a BITMAP
 *
 * CHANGES:
 * ++eero, 8/96:
 * - fixed IMG decoding (IMGs are byte aligned, BITMAPs now long aligned).
 * ++eero, 5/97:
 * - Added byte conversions for header values.
 * - changed error messages to trace stuff.
 * ++eero, 7/98:
 * - speeded up img decompressing.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>		/* byte conversion stuff */
#include <sys/stat.h>
#include "Wlib.h"
#include "proto.h"

/*
 * IMG header definition
 */

typedef struct {
  ushort ver;		/* version (1) */
  ushort hdrlen;	/* header lenght as shorts (8) */
  ushort planes;	/* number of bitplanes     (1) */
  ushort patlen;	/* lenght of patterns      (2) */
  ushort pixwidth;	/* pixel width in um       (372) */
  ushort pixheight;	/* pixel height in um      (372) */
  ushort scanwidth;	/* pixels / line (= (w+7)/8 bytes) */
  ushort scanlines;	/* total number of lines */
} IMGHDR;


/*
 * routines for loading a monochrome IMG picture
 */

static ushort *load(const char *fname, const char **error)
{
  struct stat st;
  ushort *ptr;
  short fh;

  if (stat(fname, &st)) {
    *error = "can't stat %s\n", fname;
    return NULL;
  }

  if (st.st_size < sizeof(IMGHDR)) {
    *error = "file too short";
    return NULL;
  }

  if (!(ptr = malloc(st.st_size))) {
    *error = "file alloc failed";
    return NULL;
  }

  if ((fh = open(fname, O_RDONLY, 0)) < 0) {
    *error = "file open failed";
    free(ptr);
    return NULL;
  }

  if (read(fh, ptr, st.st_size) != st.st_size) {
    *error = "file load failed";
    close(fh);
    free(ptr);
    return NULL;
  }

  close(fh);
  return ptr;
}


static long decode(uchar *data, short scanbytes, short patlen,
                   short realbytes, short scanlines, uchar *pic)
{
  short cnt, i, bytecnt;
  uchar first, *repptr = data, repcnt = 0;

  realbytes -= scanbytes;

  while (scanlines--) {

    if (repcnt) {
      repcnt--;
      data = repptr;
    }

    /* is there a repetition header? */

    if (!*data && !*(data+1) && (*(data+2) == 0xff)) {
      data += 3;
      repcnt = *data++;
      repptr = data;
    }

    bytecnt = scanbytes;
    while (bytecnt > 0)
      switch (first = *data++) {

      case 0: /* pattern run */
        cnt = *data++;
	bytecnt -= (cnt * patlen);
	while (--cnt >= 0) {
	  for (i = 0; i < patlen; i++)
	    *pic++ = *(data+i);
        }
	data += patlen;
	break;

      case 128: /* bit string */
	cnt = *data++;
	bytecnt -= cnt;
	while (--cnt >= 0)
	  *pic++ = *data++;
	break;

      default: /* solid run */
	cnt = first & 127;
	bytecnt -= cnt;
	if (first & 128) {
	  while (--cnt >= 0)
	    *pic++ = 255;
	} else {
	  while (--cnt >= 0)
	    *pic++ = 0;
        }
    }
    if(bytecnt)
      return 1;		/* error: line encoded wrong */

    pic += realbytes;
  }

  return 0;
}


/*
 * what we're going to export...
 */

BITMAP *w_readimg(const char *fname, short *width, short *height)
{
  ushort *file, *data;
  uchar *picture;
  IMGHDR *imghdr;
  BITMAP *ret;
  const char *error;
  int idx;

  TRACESTART();

  if (!(file = load(fname, &error))) {
    TRACEPRINT(("w_readimg(%s,%p,%p) -> %s\n", fname, width, height, error));
    TRACEEND();
    return NULL;
  }

  data = file;
  for (idx = 0; idx < 8; idx++) {
	 *data = ntohs(*data);
	 data++;
  }
  imghdr = (IMGHDR *)file;

  if (imghdr->ver != 1 || imghdr->hdrlen < 8 || imghdr->planes != 1) {
    free(file);
    TRACEPRINT(("w_readimg(%s,%p,%p) -> not a monochrome IMG\n", fname, width, height));
    TRACEEND();
    return NULL;
  }

  /* enough memory to load picture? */
  ret = w_allocbm(imghdr->scanwidth, imghdr->scanlines, BM_PACKEDMONO, 2);
  if (!ret) {
    free(file);
    TRACEPRINT(("w_readimg(%s,%p,%p) -> bm alloc failed\n", fname, width, height));
    TRACEEND();
    return NULL;
  }

  picture = (uchar *)(file + imghdr->hdrlen);
  if (decode(picture, (imghdr->scanwidth + 7) >> 3, imghdr->patlen,
	     ret->unitsize * ret->upl, ret->height, ret->data) < 0) {
    w_freebm(ret);
    free(file);
    TRACEPRINT(("w_readimg(%s,%p,%p) -> IMG decode failed\n", fname, width, height));
    TRACEEND();
    return NULL;
  }

  free(file);
  *width = ret->width;
  *height = ret->height;

  TRACEPRINT(("w_readimg(%s,%p,%p) -> %p\n", fname, width, height, ret));
  TRACEEND();
  return ret;
}

