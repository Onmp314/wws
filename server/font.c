/*
 * font.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- font loading and handling stuff
 *
 * CHANGES
 * ++eero, 11/96:
 * - modified to work with the new font format.
 * - besides FONTPATH, fonts can be loaded from 'glob_fontpath'.
 *   to do this I malloc space for the full pathname, you could
 *   change this to static if you'll want to.
 * ++eero, 10/97:
 * - unloadfont frees font->name too.
 * ++eero, 2/98:
 * - calculate italic correction here instead of using font header values.
 *   It's easier/safer that way.
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"


/*
 * global variables
 */

FONT glob_font[MAXFONTS];


/*
 * some private functions
 */

static int load_font(int font_h, const char *fname)
{
  int fh, i, width, height, baseline, isLittleEndian = 0;
  FONT *font = &glob_font[font_h];
  FONTHDR *hdr = &font->hdr;
  long longs, needs;
  const char *path;
  char *buf;
  ulong skew;

#ifndef FONTDIR
#error you must define FONTDIR!!!
#endif

  if (glob_fontpath) {
    path = glob_fontpath;
  } else {
    path = FONTDIR;
  }
  needs = strlen(path);
  buf = malloc(needs + strlen(fname) + 2);
  strcpy(buf, path);
  buf[needs++] = '/';
  strcpy(buf + needs, fname);

  fh = open(buf, O_RDONLY);
  free(buf);
  if (fh < 0) {
    return -1;
  }

  if (read(fh, hdr, sizeof(FONTHDR)) != sizeof(FONTHDR)) {
    close(fh);
    return -2;
  }

  /* hey, we may be running on an Intel box!
   */
  switch (hdr->magic) {
    case 0x314e4657:
      isLittleEndian = 1;
      /* fall through */
    case 0x57464e31:
      /* font header / data size */
      hdr->header       = ntohs(hdr->header) - sizeof(FONTHDR);
      hdr->lenght       = ntohl(hdr->lenght);
      /* font type information */
      hdr->height       = ntohs(hdr->height);
      hdr->flags        = ntohs(hdr->flags);
      hdr->styles       = ntohs(hdr->styles);
      /* font effect generation vars */
      hdr->thicken      = ntohs(hdr->thicken);
      hdr->skew         = ntohs(hdr->skew);
      hdr->effect_mask  = ntohs(hdr->effect_mask);
      /* font character cell size */
      hdr->baseline     = ntohs(hdr->baseline);
      hdr->maxwidth     = ntohs(hdr->maxwidth);
      break;
    case 0x544e4657:
    case 0x57464e54:
      fprintf(stderr, "'%s' is a W1R3 format font\r\n", fname);
    default:
      close(fh);
      return -3;
  }


  height = hdr->height;
  baseline = hdr->baseline;

  /* check font header value validities as faulty ones may chrash the server */
  if (hdr->header < 0 || height < 1 || height > 300 || baseline < 0) {
    fprintf(stderr, "'%s' font header has bogus value(s)\r\n", fname);
  }

  if (--baseline < 0) {
    baseline = 0;
  } else {
    if (baseline >= height) {
       baseline = height - 1;
    }
  }
  skew = hdr->skew;
  width = 0;

  /* calculate the slanting offsets */
  for (i = 0; i < height; i++) {
    if (skew & 1) {
      width++;
      skew |= 0x10000;
    }
    skew >>= 1;
    if (i == baseline) {
      font->slant_offset = -width;
    }
  }
  font->slant_offset += width;
  font->slant_size = width;

  if (hdr->header) {
    /* disregard font information */
    lseek(fh, hdr->header, SEEK_CUR);
  }

  if (read(fh, font->widths, 256) != 256) {
    close(fh);
    return -4;
  }

  longs = 0;
  for (i = 0; i < 256; i++) {
    font->offsets[i] = longs;
    longs += ((font->widths[i] * hdr->height + 31) & ~31) >> 5;
  }

  needs = longs << 2;
  /* one long is needed to prevent SIGSEGV in clipped printchar */
  if (needs != hdr->lenght || !(font->data = malloc(needs + 4))) {
    close(fh);
    return -5;
  }

  if (read(fh, font->data, needs) != needs) {
    close(fh);
    free(font->data);
    return -6;
  }

  if (isLittleEndian) {
    i = longs;
    while (--i >= 0) {
      font->data[i] = ntohl(font->data[i]);
    }
  }

  font->name = strdup(fname);
  font->numused = 1;
  close(fh);

  return font_h;
}


/*
 * the exported functions
 */

int font_init(const char *titlefont, const char *menufont)
{
  int i, err1, err2;

  if(!(titlefont && menufont)) {
    fprintf(stderr, "fatal: font initialisation error (%s/%s)\r\n", titlefont, menufont);
    return -1;
  }

  i = MAXFONTS;
  while (--i >= 0) {
    glob_font[i].numused = 0;
  }

  err1 = load_font(TITLEFONT, titlefont);
  err2 = load_font(MENUFONT, menufont);

  if ((err1 < 0) || (err2 < 0)) {
    fprintf(stderr, "fatal: font initialisation error (%d/%d)\r\n", err1, err2);
    return -1;
  }

  return 0;
}


short font_loadfont(const char *fontname)
{
  short i, fglobh;

  fglobh = -1;
  for (i = 0; i < MAXFONTS; i++) {
    if (!glob_font[i].numused) {
      fglobh = i;
      break;
    }
  }

  if (fglobh == -1) {
    return -1;
  }

  return load_font(fglobh, fontname);
}


short font_unloadfont(FONT *fp)
{
  if (fp->numused > 0) {
    return -1;
  }

  fp->numused = 0;
  free(fp->name);
  free(fp->data);

  return 0;
}


/*
 * calculate the length of a string in pixel
 */

int fontStrLen(FONT *f, const uchar *s)
{
  int len = 0;

  while (*s) {
    /*
     * kay: I have those sign extension bugs!
     */
    len += f->widths[*s++ & 0xff];
  }

  return len;
}
