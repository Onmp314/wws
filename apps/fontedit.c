/*
 * fontedit.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a utility for handling W fonts.
 *
 * NOTE
 * - Values in the W font headers are in big endian (network) order.
 * - baseline should be an offset into a lowest line having pixels in
 *   alphanumeric characters (besides [qypj]).  Underline will be two lines
 *   lower.  Baseline will also be used in calculating the italic
 *   correction.
 *
 * LIMITATIONS
 * - W1R3 font format can be loaded, but save format is always the new one.
 * - Operations on font data are supported only for fonts which maximum
 *   character width is 32 pixels.
 * - Works on little and big endian machines with 32-bit longs and 16-bit
 *   shorts.
 *
 * CHANGES
 * - Moved slanting calculations to the server for safety reasons.
 * - To make sure that font names correspond to their attributes,
 *   this saves the font itself instead of outputting it to stdout.
 *
 * TODO
 * - testing the program functionality more throughly.
 * - test and find best options for the current W fonts.
 * - Apply_effects() F_LIGHT pattern (16x16 bits) should be user selectable.
 * - create the (256 byte) font charset conversion map files:
 *   Atari->latin1, DOS->latin1, TeX->latin1 etc..
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <netinet/in.h>		/* byte order conversions */
#include <ctype.h>
#include "Wlib.h"		/* font flags */
#include "../server/types.h"	/* font file header */
#include "../server/config.h"	/* font restrictions */

/* font generation IDs */
#define OLD_MAGIC	0x57464e54
#define MAGIC		0x57464e31


/* operation ranges */
static int RangeStart = 0;
static int RangeEnd = 256;


#define CHARSET(flags,set) ((flags & ~(F_LATIN1|F_ASCII|F_SYMBOL)) | set)


typedef struct
{
  FONTHDR header;

  short top, bottom;		/* font cell size */
  short left, right;
  short slant_offset;		/* slanting offsets */
  short slant_size;

  char *info;			/* font info (author etc.) */
  short info_size;
  uchar widths[256];		/* induvidual character widths */
  ulong *data;			/* long aligned character bitstream */
  ulong *map;			/* font bitmaps */
} MEMFONT;


/* ----------------
 * font data manipulation
 */

/* set font maxwidth and proportionality according to font flags and widths */
static void check_maxwidth(MEMFONT *font)
{
  int idx, width, start = 0, end = 256, max = 0;

  if(font->header.flags & (F_ASCII | F_LATIN1))
    start = 32;
  if(font->header.flags & F_ASCII)
    end = 127;

  font->header.flags &= ~F_PROP;
  for(idx = start; idx < end; idx++)
  {
    width = font->widths[idx];
    if(width > max)
    {
      if(max)
        font->header.flags |= F_PROP;
      max = width;
    }
  }
  font->header.maxwidth = max;
}


/* read new charset order from file (256 bytes) and apply it */
static void order_charset(MEMFONT *font, char *fname)
{
  FILE *fp;
  uchar mapping[256], tmp;
  short idx, h, height = font->header.height;
  ulong *map, *sptr, *dptr;

  if(!(fp = fopen(fname, "rb")))
  {
    fprintf(stderr, "unable to access `%s' charset mapping\n", fname);
    return;
  }
  if(fread(mapping, 1, 256, fp) != 256)
  {
    fprintf(stderr, "charset map `%s' reading failed\n", fname);
    return;
  }
  if(!(map = calloc(256 * font->header.height, sizeof(long))))
  {
    fprintf(stderr, "unable to allocate new font map\n");
    return;
  }
  sptr = font->map;
  for(idx = 0; idx < 256; idx++)
  {
    tmp = font->widths[mapping[idx]];
    font->widths[mapping[idx]] = font->widths[idx];
    font->widths[idx] = tmp;

    dptr = map + mapping[idx] * height;
    for(h = 0; h < height; h++)
      *dptr++ = *sptr++;
  }
  free(font->map);
  font->map = map;
}


/* insert long aligned raw bitmap from `fname' */
static void insert_bitmap(MEMFONT *font, char *fname)
{
  struct stat st;
  short count, height = font->header.height;
  FILE *fp;

  if(stat(fname, &st) < 0 || !(fp = fopen(fname, "rb")))
  {
    fprintf(stderr, "unable to access bitmap `%s'\n", fname);
    return;
  }
  count = st.st_size / height;
  if(RangeStart + count > RangeEnd)
    count = RangeEnd - RangeStart;
  count *= height;
  fread(font->map + RangeStart * height, sizeof(long), count, fp);
}


/* calculate slant offsets from the font skew pattern, height and baseline */
static void get_slanting(MEMFONT *font)
{
  int width, right, height, baseline;
  FONTHDR *header = &(font->header);
  ulong skew = header->skew;

  baseline = header->baseline;
  width = right = 0;
  height = -1;

  while(++height < header->height)
  {
    if(skew & 1)
    {
      skew |= 0x10000;
      width++;
    }
    skew >>= 1;
    if(height == baseline)
      right = width;
  }
  font->slant_offset = width - right;
  font->slant_size = width;
}


/* apply style effects to the font */
static void apply_effects(MEMFONT *font, ushort styles)
{
  static ushort pattern[16] = {
    0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555
  };
  ushort val, bolder, reverse, italic, olds, *lighten = NULL;
  int idx, monospaced, start = 0, end = 256;
  short width, h, height, underline;
  ulong bits, *ptr;

  monospaced = !(font->header.flags & F_PROP);
  styles &= font->header.effect_mask;
  olds = font->header.styles;

  if((styles & F_BOLD) && (monospaced ||
     font->header.maxwidth + ((font->header.thicken) >> 1) < 32))
  {
    bolder = font->header.thicken;
    olds |= F_BOLD;
  }
  else
    bolder = 0;

  if(styles & F_LIGHT)
  {
    lighten = pattern;
    olds |= F_LIGHT;
  }
  else
    lighten = 0;

  if(styles & F_REVERSE)
  {
    reverse = 1;
    olds |= F_REVERSE;
  }
  else
    reverse = 0;

  if(styles & F_UNDERLINE)
  {
    if((underline = font->header.baseline + 2) >= font->header.height)
      underline = font->header.height - 3;
    olds |= F_UNDERLINE;
  }
  else
    underline = 0;

  /* not adviced for precalculation */
  italic = 0;
  if(styles & F_ITALIC)
  {
    get_slanting(font);
    if (font->header.maxwidth + font->slant_size < 32)
    {
      italic = font->header.skew;
      olds |= F_ITALIC;
    }
  }

  font->header.effect_mask &= ~olds;
  font->header.styles = olds;
  styles &= olds;
  if(!styles)
    return;

  if(font->header.flags & (F_ASCII | F_LATIN1))
    start = 32;
  if(font->header.flags & F_ASCII)
    end = 127;

  height = font->header.height;
  ptr = font->map + start * height;
  for(idx = start; idx < end; idx++)
  {
    width = font->widths[idx];
    if(!width)
      continue;

    bits = (1 << width) - 1;
    if(bolder)				/* F_BOLD */
    {
      /* thicken vertically */
      val = bolder >> 1;
      while(val-- > 0)
      {
	for(h = 1; h < height; h++)
	  ptr[h-1] |= ptr[h];
      }
      /* thicken horizontally */
      val = (bolder + 1) >> 1;
      while(val-- > 0)
      {
	for(h = 0; h < height; h++)
	  ptr[h] |= (ptr[h] << 1);
      }
      if(monospaced)
      {
	for(h = 0; h < height; h++)
	  ptr[h] &= bits;
      }
      else
      {
        /* presume programs using proportional fonts can deal with
	 * different styles of same font having different widths.
	 */
	font->widths[idx] = width + ((bolder + 1) >> 1);
      } 
    }

    if(lighten)				/* F_LIGHT */
    {
      for(h = 0; h < height; h++)
      {
        ptr[h] &= lighten[h&15];
      }
    }

    if(underline)			/* F_UNDERLINE */
      ptr[underline] |= bits;

    if(reverse)				/* F_REVERSE */
    {
      for(h = 0; h < height; h++)
        ptr[h] ^= bits;
    }

    if(italic)				/* F_ITALIC */
    {
      val = 0;
      bits = italic;
      for(h = 0; h < height; h++)
      {
        if(bits & 1)
	{
	  bits |= 0x10000;
	  val++;
	}
	bits >>= 1;
        ptr[h] <<= val;
      }
      font->widths[idx] += val;
    }

    ptr += height;
  }
}


/* scroll font according to dir characters (u,d,l,r) */
static void scroll_font(MEMFONT *font, char *dirs)
{
  int vert = 0, horz = 0, idx, count, h, height, width;
  ulong *ptr, bits;

  while(*dirs)
  {
    switch(*dirs)
    {
      case 'u': vert--; break;
      case 'd': vert++;	break;
      case 'l': horz--;	break;
      case 'r': horz++;	break;
    }
    dirs++;
  }
  if(!(vert || horz))
    return;

  height = font->header.height;
  ptr = font->map + RangeStart * height;
  vert %= height;

  for(idx = RangeStart; idx < RangeEnd; idx++)
  {
    /* scroll down */
    count = vert;
    while(count-- > 0)
    {
      bits = ptr[height-1];
      for(h = height-1; h > 0; h--)
	ptr[h] = ptr[h-1];
      ptr[0] = bits;
    }
    /* scroll up */
    count = -vert;
    while(count-- > 0)
    {
      bits = ptr[0];
      for(h = 1; h < height; h++)
	ptr[h-1] = ptr[h];
      ptr[height-1] = bits;
    }
    /* scroll right */
    count = horz;
    if(count > 0)
    {
      width = font->widths[idx];
      if(!width)
        continue;
      bits = (1 << width) - 1;
      count %= width;
      width -= count;
      for(h = 0; h < height; h++)
	ptr[h] = ((ptr[h] << width) & bits) | (ptr[h] >> count);
    }
    /* scroll left */
    count = -horz;
    if(count > 0)
    {
      width = font->widths[idx];
      if(!width)
        continue;
      bits = (1 << width) - 1;
      count %= width;
      width -= count;
      for(h = 0; h < height; h++)
	ptr[h] = (ptr[h] >> width) | ((ptr[h] << count) & bits);
    }
    ptr += height;
  }
}


/* change font width in the bitmaps. add/delete pixels to/from right edge. */
static void change_widths(MEMFONT *font, int change)
{
  short max, width, idx, height, h, narrow;
  ulong *ptr;

  max = 0;
  for(idx = RangeStart; idx < RangeEnd; idx++)
  {
    width = font->widths[idx];
    if(width > max)
      max = width;
  }

  if(max + change <= 0 || max + change > 32)
  {
    fprintf(stderr, "character widths cannot be changed by %d!\n", change);
    return;
  }

  if(change < 0)
  {
    change = -change;
    narrow = change;
  }
  else
    narrow = 0;

  height = font->header.height;
  ptr = font->map + RangeStart * height;
  for(idx = RangeStart; idx < RangeEnd; idx++)
  {
    width = font->widths[idx];
    if(width > 0 && width - narrow > 0)
    {
      if(narrow)
      {
        width -= change;
        for(h = 0; h < height; h++)
	{
	  *ptr >>= change;
	  ptr++;
	}
      }
      else
      {
        width += change;
        for(h = 0; h < height; h++)
	{
	  *ptr <<= change;
	  ptr++;
	}
      }
    }
    else
    {
      width = 0;
      for(h = 0; h < height; h++)
        *ptr++ = 0;
    }
    font->widths[idx] = width;
  }
  check_maxwidth(font);
}


/* Check character widths.  After calculating new widths you'll probably
 * have to increase the font widths so that fonts will not be packed too
 * tightly together!
 */
static void check_widths(MEMFONT *font)
{
  short idx, right, left, height, width, h, w, last;
  ulong bits, *ptr;

  height = font->header.height;
  ptr = font->map + RangeStart * height;
  for(idx = RangeStart; idx < RangeEnd; idx++)
  {
    right = left = width = 32;		/* maximum values */
    for(h = 0; h < height; h++)
    {
      bits = *ptr++;
      if(bits)
      {
	last = 0;
	for(w = 0; w < width; w++)
	{
	  if(bits & 1)
	  {
	    if(w < right)
	      right = w;
	    last = w;
	  }
	  bits >>= 1;
	}
	last = width - 1 - last;
	if(last < left)
	  left = last;
      }
    }
    if((width -= right + left) < 0)
      width = 0;

    /* right align character bits */
    if(right)
    {
      ptr -= height;
      for(h = 0; h < height; h++)
	*ptr++ >>= right;
    }
    font->widths[idx] = width;
  }
  check_maxwidth(font);
}


/* Set font width.  Result will be a monospaced font where characters are
 * centered to the character cell.  Width 0 will create an `empty' font.
 */
static void set_widths(MEMFONT *font, int nwidth)
{
  short idx, right, left, height, width, h, w, last;
  ulong bits, mask, *ptr;

  if(!(font->header.flags & F_PROP) && nwidth == font->header.maxwidth)
  {
    fprintf(stderr, "width already %d!\n", nwidth);
    return;
  }

  height = font->header.height;
  if(nwidth <= 0)
  {
    fprintf(stderr, "Emptying the characters %d - %d!\n", RangeStart, RangeEnd);
    memset(font->map + RangeStart * height, 0, height * (RangeEnd-RangeStart) * sizeof(long));
    memset(font->widths + RangeStart, 0, RangeEnd - RangeStart);
    return;
  }

  if(nwidth < 2 || nwidth > 32)
  {
    fprintf(stderr, "character widths cannot be changed to %d!\n", nwidth);
    return;
  }

  mask = (1 << nwidth) - 1;
  ptr = font->map + RangeStart * height;
  for(idx = RangeStart; idx < RangeEnd; idx++)
  {
    /* first check horizontal extent for char */
    right = left = width = 32;
    for(h = 0; h < height; h++)
    {
      bits = *ptr++;
      if(bits)
      {
	last = 0;
	for(w = 0; w < width; w++)
	{
	  if(bits & 1)
	  {
	    if(w < right)
	      right = w;
	    last = w;
	  }
	  bits >>= 1;
	}
	last = width - 1 - last;
	if(last < left)
	  left = last;
      }
    }
    if((width -= right + left) < 0)
    {
      /* empty char */
      font->widths[idx] = 0;
      continue;
    }

    /* right align char */
    if(right)
    {
      ptr -= height;
      for(h = 0; h < height; h++)
      {
	*ptr = *ptr >> right;
	ptr++;
      }
    }

    font->widths[idx] = nwidth;
    if(width == nwidth)
      continue;

    /* center char into `nwidth' */
    ptr -= height;
    if(width < nwidth)
    {
      left = (nwidth - width) / 2;
      for(h = 0; h < height; h++)
      {
	*ptr <<= left;
	ptr++;
      }
    }
    else
    {
      right = (width - nwidth) / 2;
      for(h = 0; h < height; h++)
      {
	*ptr >>= right;
	*ptr++ &= mask;
      }
    }
  }
  check_maxwidth(font);
}


/* change font height */
static void set_height(MEMFONT *font, short new)
{
  ulong *map, *nptr, *optr;
  short move, idx, h, old;

  map = calloc(256 * new, sizeof(long));
  if(!map)
  {
    fprintf(stderr, "not enough memory for new font map\n");
    return;
  }
  nptr = map;
  optr = font->map;
  old = font->header.height;
  if(old < new)
    move = old;
  else
    move = new;

  for(idx = 0; idx < 256; idx++)
  {
    for(h = 0; h < move; h++)
      nptr[h] = optr[h];
    nptr += new;
    optr += old;
  }
  free(font->map);
  font->map = map;
  font->header.height = new;
  if(font->header.baseline + 2 >= new)
  {
    font->header.baseline = new - new/4;
    if(font->header.baseline + 2 >= new)
      font->header.baseline = new - 3;
  }      
}


/* check character cell size */
static void check_cell(MEMFONT *font)
{
  short top, bottom, left, right, height, width, h, w, last, line;
  int idx, start = 0, end = 256;
  ulong bits, *ptr;

  ptr = font->map;
  if(!ptr)
  {
    fprintf(stderr, "font stream unmapped!\n");
    return;
  }

  if(font->header.flags & (F_ASCII | F_LATIN1))
    start = 32;
  if(font->header.flags & F_ASCII)
    end = 127;

  height = font->header.height;
  ptr += start * height;

  left = right = font->header.maxwidth;
  top = bottom = height;

  for(idx = start; idx < end; idx++)
  {
    width = font->widths[idx];	/* check left/right */
    if(!width)
      continue;

    line = 0;
    for(h = 0; h < height; h++)
    {
      bits = *ptr++;
      if(bits)
      {
	last = 0;
	for(w = 0; w < width; w++)
	{
	  if(bits & 1)
	  {
	    if(w < left)
	      left = w;
	    last = w;
	  }
	  bits >>= 1;
	}
	last = width - 1 - last;
	if(last < right)
	  right = last;

	if(h < top)
	  top = h;
	line = h;
      }
    }
    line = height - 1 - line;
    if(line < bottom)
      bottom = line;
  }

  font->top    = top;
  font->bottom = bottom;
  font->left   = left;
  font->right  = right;
}


/* ---------------------------
 * font data stream<->bitmap conversions
 */

static ulong *stream2map(MEMFONT *font)
{
  register ulong *cptr, cbit, cdata, *dptr, dbit, ddata;
  register short cheight, lcwidth;
  short cwidth, height;
  ushort idx;

  height = font->header.height;
  if(font->map)
    free(font->map);
  font->map = (ulong*)calloc(256 * height, sizeof(long));
  if(!(font->map && font->data))
  {
    fprintf(stderr, "font data stream mapping failed (memory?)!\n");
    return NULL;
  }

  dptr = font->map;
  cptr = font->data;
  cdata = cbit = 0;

  for(idx = 0; idx < 256; idx++)
  {
    cwidth = font->widths[idx];
    if(!cwidth)
    {
      dptr += height;
      continue;
    }

    if(cwidth > 32)
    {
      fprintf(stderr, "character more than 32 pixels wide!\n");
      return NULL;
    }

    if(cbit != 0x80000000)
    {
      cbit = 0x80000000;
      cdata = *cptr++;
    }
    cheight = height;
    while (--cheight >= 0)
    {
      ddata = 0;
      dbit = 1 << (cwidth-1);
      lcwidth = cwidth;
      while (lcwidth-- > 0)
      {
	if (cdata & cbit)
	  ddata |= dbit;

	dbit >>= 1;
	if (!(cbit >>= 1))
	{
	  cbit = 0x80000000;
	  cdata = *cptr++;
	}
      }
      *dptr++ = ddata;
    }
  }
  return font->map;
}


static ulong *map2stream(MEMFONT *font)
{
  register ulong *cptr, cbit, cdata, *dptr, dbit, ddata;
  register short cheight, lcwidth;
  short cwidth, height;
  ushort idx;
  long longs;

  if(font->data)
    free(font->data);

  longs = 0;
  height = font->header.height;
  for (idx = 0; idx < 256; idx++)
    longs += (font->widths[idx] * height + 31) >> 5;

  longs = longs * sizeof(long);
  font->data = malloc(longs);
  font->header.lenght = longs;

  if(!(font->map && font->data))
  {
    fprintf(stderr, "font bitmap streaming failed!\n");
    return NULL;
  }

  memset(font->data, 0, longs);
  dptr = font->map;
  cptr = font->data;
  cbit = 0;

  for(idx = 0; idx < 256; idx++)
  {
    cwidth = font->widths[idx];
    if(!cwidth)
    {
      dptr += height;
      continue;
    }

    cdata = 0;
    cbit = 0x80000000;
    cheight = height;
    while (--cheight >= 0)
    {
      dbit = 1 << (cwidth-1);
      ddata = *dptr;
      lcwidth = cwidth;
      while (lcwidth-- > 0)
      {
        if (ddata & dbit)
	  cdata |= cbit;

	dbit >>= 1;
	if (!(cbit >>= 1))
	{
	  cbit = 0x80000000;
	  *cptr++ = cdata;
	  cdata = 0;
	}
      }
      dptr++;
    }
    if(cbit != 0x80000000)
      *cptr++ = cdata;
  }
  free(font->map);
  font->map = NULL;
  return font->data;
}


/* ----------------
 * font header manipulation
 */

/* parse special font flags: proportional, symbol font... */
static ushort parse_flags(char *str)
{
  ushort flags = 0;

  for(;;)
    switch(tolower(*str++))
    {
      case 'p':
        flags |= F_PROP;
	break;
      case 'l':
        flags = CHARSET(flags, F_LATIN1);
	break;
      case 'a':
        flags = CHARSET(flags, F_ASCII);
	break;
      case 's':
        flags = CHARSET(flags, F_SYMBOL);
	break;
      default:
        return flags;
    }
}


/* parse font style flags */
static ushort parse_styles(char *str)
{
  ushort flags = 0;

  for(;;)
    switch(tolower(*str++))
    {
      case 'b':
        flags |= F_BOLD;
	break;
      case 'i':
        flags |= F_ITALIC;
	break;
      case 'l':
        flags |= F_LIGHT;
	break;
      case 'r':
        flags |= F_REVERSE;
	break;
      case 'u':
        flags |= F_UNDERLINE;
	break;
      default:
        return flags;
    }
}


/* copy lowercased name to the header */
static void set_family(FONTHDR *header, char *name)
{
  int i = strlen(name);
  while(--i >= 0 && name[i] != '/');
  name = &name[++i];
  for(i = 0; isalpha(name[i]) && i < sizeof(header->family)-1; i++)
    header->family[i] = tolower(name[i]);
  header->family[i] = '\0';
}


/* -----------------
 * header conversions
 */

/* set new header variables for the old W fonts */
static void old_defaults(FONTHDR *header, char *file)
{
  short height = header->height;

  header->magic = MAGIC;
  header->header = sizeof(FONTHDR);

  /* data lenght set later */
  set_family(header, file);
  header->flags = F_LATIN1;
  /* height, flags and styles already set */

  header->baseline = height - height/4;
  if(header->baseline + 2 >= height)
    header->baseline = height - 3;

  if(height < 8)
    header->effect_mask = F_REVERSE|F_UNDERLINE;
  else
  {
    header->effect_mask = F_STYLEMASK;		/* all effects */
    if (height < 14)
    {
      header->effect_mask ^= F_LIGHT;		/* except dimmed */
      if (height < 10)
	      header->effect_mask ^= F_ITALIC;	/* and slanted */
    }
    header->thicken = 1;
    header->skew = 0xAAAA;
  }
}


/* Convert header variable byte orders between big<->little endian.
 * This expects the same byte order swapping function to work both
 * ways as it does on Intel processors (Motorola already use network
 * byteorder so there it's simple a nop)
 */
static void convert_header(FONTHDR *header)
{
  /* font header / data size */
  header->magic  = ntohl(header->magic);
  header->lenght = ntohl(header->lenght);
  header->header = ntohs(header->header);

  /* font type information */
  header->height = ntohs(header->height);
  header->flags  = ntohs(header->flags);
  header->styles = ntohs(header->styles);

  /* font effect generation vars */
  header->thicken      = ntohs(header->thicken);
  header->skew         = ntohs(header->skew);
  header->effect_mask  = ntohs(header->effect_mask);

  /* font character cell size */
  header->baseline = ntohs(header->baseline);
  header->maxwidth = ntohs(header->maxwidth);
}


/* ---------------
 * load / save fonts
 */

static MEMFONT *load_font(char *file)
{
  static MEMFONT font;
  long longs, idx;
  FILE *fp = stdin;

  /* '-' = stdin, otherwise should be filename */
  if (file[0] != '-' || file[1])
    fp = fopen(file, "rb");

  if (!fp)
  {
    fprintf(stderr, "font `%s' doesn't exist!\n", file);
    return NULL;
  }
  memset(&font, 0, sizeof(MEMFONT));

  fread(&font.header.magic, sizeof(long), 1, fp);
  switch(ntohl(font.header.magic))
  {
    case OLD_MAGIC:
      /* no need to convert magic, it will be new */
      fread(&font.header.styles, sizeof(short), 1, fp);
      fread(&font.header.height, sizeof(short), 1, fp);
      font.header.styles = ntohs(font.header.styles) & F_STYLEMASK;
      font.header.height = ntohs(font.header.height);
      old_defaults(&font.header, file);
      break;

    case MAGIC:
      idx = sizeof(font.header.magic);
      fread((char*)&font.header + idx, 1, sizeof(FONTHDR) - idx, fp);

      /* converts everything */
      convert_header(&font.header);

      font.info_size = font.header.header - sizeof(FONTHDR);
      if(font.info_size && (font.info = (char*)malloc(font.info_size)))
        fread(font.info, 1, font.info_size, fp);
      break;

    default:
      fprintf(stderr, "`%s' is not a W font!\n", file);
      fclose(fp);
      return NULL;
  }

  if(fread(font.widths, 1, 256, fp) != 256)
  {
    fprintf(stderr, "font width table loading failed!\n");
    fclose(fp);
    return NULL;
  }
  check_maxwidth(&font);

  longs = 0;
  for (idx = 0; idx < 256; idx++)
    longs += (font.widths[idx] * font.header.height + 31) >> 5;

  font.header.lenght = longs * sizeof(long);
  if(!(font.data = (ulong*)malloc(font.header.lenght)))
  {
    fprintf(stderr, "not enough memory for font data!\n");
    fclose(fp);
    return NULL;
  }

  if(fread(font.data, sizeof(long), longs, fp) != longs)
  {
    fprintf(stderr, "font data loading failed!\n");
    fclose(fp);
    return NULL;
  }
  /* set proper byte order */
  for(idx = 0; idx < longs; idx++)
    font.data[idx] = ntohl(font.data[idx]);

  return &font;
}


/* compose a font name that server expects of font with given attributes */
static char *font2name(const char *family, short size, short styles)
{
  static char fname[MAXFAMILYNAME + 5 + 3 + 1 + sizeof(WFONT_EXTENSION) + 1];
  short idx = 0;

  /* family name */
  if (!*family) {
    fprintf(stderr, "Font family name missing!\n");
    return NULL;
  }
  do {
    fname[idx++] = *family++;
  } while(idx < MAXFAMILYNAME && *family > 32);

  /* size */
  if (size < 1) {
    fprintf(stderr, "Illegal font size!\n");
    return NULL;
  }
  if (size / 100) {
    fname[idx++] = '0' + size / 100;
    size %= 100;
  }
  if (size / 10) {
    fname[idx++] = '0' + size / 10;
    size %= 10;
  }
  fname[idx++] = '0' + size;

  /* styles */
  if (styles & F_BOLD) {
    fname[idx++] = 'b';
  }
  if (styles & F_ITALIC) {
    fname[idx++] = 'i';
  }
  if (styles & F_LIGHT) {
    fname[idx++] = 'l';
  }
  /* underline and reverse should be generally done with effects */
  if (styles & F_REVERSE) {
    fname[idx++] = 'r';
  }
  if (styles & F_UNDERLINE) {
    fname[idx++] = 'u';
  }

  /* extension */
  fname[idx++] = '.';
  family = WFONT_EXTENSION;
  while(*family && idx < sizeof(fname)) {
    fname[idx++] = *family++;
  }
  fname[idx] = '\0';

  return fname;
}


static void save_font(MEMFONT *font)
{
  long idx, longs = font->header.lenght / sizeof(long);
  char *name;
  FILE *fp;

  name = font2name(font->header.family, font->header.height, font->header.styles);
  if (!(name && (fp = fopen(name, "wb"))))
  {
    fprintf(stderr, "Saving of font '%s' failed.\n", name);
    return;
  }

  check_maxwidth(font);
  convert_header(&font->header);
  fwrite(&font->header, 1, sizeof(FONTHDR), fp);
  convert_header(&font->header);

  if(font->info)
    fwrite(font->info, 1, font->info_size, fp);

  fwrite(font->widths, 1, 256, fp);

  for(idx = 0; idx < longs; idx++)
    font->data[idx] = htonl(font->data[idx]);

  if(fwrite(font->data, 1, font->header.lenght, fp) != font->header.lenght)
    fprintf(stderr, "font save failed!\n");

  for(idx = 0; idx < longs; idx++)
    font->data[idx] = ntohl(font->data[idx]);

  fclose(fp);
}


/* -----------------
 * user information
 */

/* show font attributes */
static void show_attributes(MEMFONT *font)
{
  typedef struct
  {
    int flag;
    const char *name;
  } OPTION;

  static OPTION flag[] =
  {
    { F_PROP,		"proportional" },
    { F_LATIN1,		"ISO-latin1" },
    { F_ASCII,		"ASCII" },
    { F_SYMBOL,		"symbol" }
  };
  static OPTION style[] =
  {
    { F_BOLD,		"bold" },
    { F_ITALIC,		"italic" },
    { F_LIGHT,		"light" },
    { F_REVERSE,	"reverse" },
    { F_UNDERLINE,	"underline" }
  };

  FONTHDR *header = &font->header;
  int idx;

  fprintf(stderr, "\nfamily: %s\n", header->family);
  fprintf(stderr, "height: %d\n", header->height);
  fprintf(stderr, "maximum width: %d\n", header->maxwidth);
  fprintf(stderr, "baseline offset: %d\n", header->baseline);

  fprintf(stderr, "flags (0x%X):\n  ", header->flags);
  if(header->flags)
  {
    for(idx = 0; idx < 4; idx++)
      if(header->flags & flag[idx].flag)
        fprintf(stderr, "%s ", flag[idx].name);
  }
  else
    fprintf(stderr, "none");
  
  fprintf(stderr, "\nstyles (0x%X):\n  ", header->styles);
  if(header->styles)
  {
    for(idx = 0; idx < 5; idx++)
      if(header->styles & style[idx].flag)
        fprintf(stderr, "%s ", style[idx].name);
  }
  else
    fprintf(stderr, "normal");
  
  fprintf(stderr, "\nallowed effects (0x%X):\n  ", header->effect_mask);
  if(header->effect_mask)
  {
    for(idx = 0; idx < 5; idx++)
      if(header->effect_mask & style[idx].flag)
        fprintf(stderr, "%s ", style[idx].name);
  }
  else
    fprintf(stderr, "empty");

  fprintf(stderr, "\neffect variables:\n  ");
  fprintf(stderr, "boldness: %d, ", header->thicken);
  fprintf(stderr, "skew: 0x%X\n\n", header->skew);

  if(font->info)
  {
    fprintf(stderr, "additional information:\n");
    fwrite(font->info, 1, font->info_size, stderr);
    fprintf(stderr, "\n\n");
  }
}


static void show_widths(MEMFONT *font)
{
  int row, col;

  for(row = 0; row < 16; row++)
  {
    for(col = 0; col < 16; col++)
      fprintf(stderr, "%2d, ", font->widths[row*16 + col]);
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\nfont header: %d + %d + 256 bytes\n",
          (int)sizeof(FONTHDR), (int)(font->header.header - sizeof(FONTHDR)));
  fprintf(stderr, "bitstreams: %ld bytes\n", font->header.lenght);
}


static void help(void)
{
  fprintf(stderr, "\nW font converter and header editor\n\n");
  fprintf(stderr, "usage: fontconv load_font [actions] > save_font\n\n");

  fprintf(stderr, "actions handling the font header:\n");
  fprintf(stderr, " -x <string>   add extra information\n");
  fprintf(stderr, " -f [p[l|a|s]] set font flags\n");
  fprintf(stderr, " -n <name>     set font family name\n");
  fprintf(stderr, " -b #          set baseline offset\n");
  fprintf(stderr, " -s [bilru]    set font style flags\n");
  fprintf(stderr, " -e [bilru]    set font effect mask flags\n");
  fprintf(stderr, " -t #          set bold effect thickness\n");
  fprintf(stderr, " -i #          set italics skew pattern\n");
  fprintf(stderr, " -p <file>     set 16x16 bit lighten pattern\n\n");

  fprintf(stderr, "actions handling font data:\n");
  fprintf(stderr, " -a [bilru]    apply font effects\n");
  fprintf(stderr, " -m <file>     remap character set according to <file>\n");
  fprintf(stderr, " -h #          change character heights to given value\n");

  fprintf(stderr, " -r # #        set action range ([0-255] => [0-255])\n");
  fprintf(stderr, " -c [udlr]*    scroll characters (up, down, left, right)\n");
  fprintf(stderr, " -d #          change character widths by given value\n");
  fprintf(stderr, " -w #          center characters to given width\n");
  fprintf(stderr, " -y <file>     insert raw data to the range\n");
  fprintf(stderr, " -o            output raw data from the range\n\n");

  fprintf(stderr, "Actions are applied in the order they appear on the command line.\n");
  fprintf(stderr, "After actions the new font will saved with a correct name, unless\n");
  fprintf(stderr, "`-v' (#>1) or `-o' option is specified.\n\n");

  fprintf(stderr, " -v #          set verbosity level (0-3)\n");
  fprintf(stderr, " -u            check and show character cell size\n\n");
}


/* ----------
 * main
 */
int main(int argc, char *argv[])
{
  int a, idx;
  int verbose = 0, cellsize_check = 0, output_raw = 0;
  char *action;
  MEMFONT *font;

#if 0
  fprintf(stderr, "input file: %s\n", argv[1]);
#endif

  /* load font */
  if(argc < 2 || !(font = load_font(argv[1])))
  {
    help();
    return -1;
  }
  idx = 1;

  if(!stream2map(font))
    return -1;

  /* parser and execute options */
  while(++idx < argc)
  {
    action = argv[idx++];

    /* not an action or no action argument? */
    if(!(action[0] == '-' && action[2] == '\0' &&
         (idx < argc || action[1] == 'u' || action[1] == 'o')))
    {
      help();
      return -1;
    }

    switch(action[1])
    {
      /* set options for later actions */

      case 'o':
        output_raw = 1;
        idx--;
	break;

      case 'u':
	cellsize_check = 1;
        idx--;
	break;

      case 'v':
	/* 0: errors, 1: actions, 2: attributes,, 3: widths, 4: no save */
	fprintf(stderr, "show: ");
        switch((verbose = atoi(argv[idx])))
	{
	  default:
	  case 3: fprintf(stderr, "widths, ");
	  case 2: fprintf(stderr, "attributes (*disables font output*), ");
	  case 1: fprintf(stderr, "font handling, ");
	  case 0: fprintf(stderr, "errors\n");
	}
	break;

      case 'r':
        RangeStart = atoi(argv[idx]);
        if(RangeStart < 0 || RangeStart > 255 || ++idx >= argc)
	{
	  help();
	  return -1;
	}
	RangeEnd = atoi(argv[idx]);
	if(RangeEnd < RangeStart)
	  RangeEnd = RangeStart;
	if(RangeEnd > 255)
	  RangeEnd = 255;
	if(verbose)
	  fprintf(stderr, "range: %d - %d\n", RangeStart, RangeEnd);
	RangeEnd++;
	break;

      /* modify font data */

      case 'y':
        if(verbose)
          fprintf(stderr, "inserting raw data at char %d\n", RangeStart);
        insert_bitmap(font, argv[idx]);
	cellsize_check = 1;
	break;

      case 'a':
        if(verbose)
          fprintf(stderr, "applying style effects\n");
        apply_effects(font, parse_styles(argv[idx]));
	cellsize_check = 1;
	break;

      case 'h':
        a = atoi(argv[idx]);
        if(verbose)
          fprintf(stderr, "changing font height from %d to %d\n", font->header.height, a);
        set_height(font, a);
	cellsize_check = 1;
	break;

      case 'w':
        a = atoi(argv[idx]);
        if(verbose)
          fprintf(stderr, "changing character widths to %d\n", a);
        set_widths(font, a);
	cellsize_check = 1;
	break;

      case 'd':
        a = atoi(argv[idx]);
	if(a)
	{
          if(verbose)
            fprintf(stderr, "changing character widths by %d\n", a);
          change_widths(font, a);
	}
	else
	{
          if(verbose)
            fprintf(stderr, "checking character widths\n");
          check_widths(font);
	}
	cellsize_check = 1;
	break;

      case 'c':
        if(verbose)
          fprintf(stderr, "scrolling (%s) characters\n", argv[idx]);
        scroll_font(font, argv[idx]);
	cellsize_check = 1;
	break;

      case 'm':
        if(verbose)
          fprintf(stderr, "ordering font with `%s' character map\n", argv[idx]);
	order_charset(font, argv[idx]);

      /* modify just font header values */

      case 'x':				/* extra font information */
        font->info = strdup(argv[idx]);
	font->info_size = strlen(font->info) + 1;
	font->header.header = sizeof(FONTHDR) + font->info_size;
	break;

      case 'n':				/* font family name */
	set_family(&font->header, argv[idx]);
        break;

      case 'f':
	font->header.flags = parse_flags(argv[idx]);
        break;

      case 's':
	font->header.styles = parse_styles(argv[idx]);
	font->header.effect_mask &= ~font->header.styles;
        break;

      case 'e':
	font->header.effect_mask = parse_styles(argv[idx]);
	font->header.effect_mask &= ~font->header.styles;
        break;

      case 't':				/* bold effect thickness */
        a = atoi(argv[idx]);
	if(a < 0 || a > 5)
	  a = 1;
        font->header.thicken = a;
        break;

      case 'b':				/* baseline offset */
        a = atoi(argv[idx]);
	if(a < 0 || a+2 >= font->header.height)
	  a = font->header.height - 3;
	font->header.baseline = a;
        break;

      case 'i':				/* effect patterns */
        sscanf(argv[idx], "%hx", &font->header.skew);
        break;

#if 0
      case 'p':
        sscanf(argv[idx], "%hx", &font->header.lighten);
        break;
#endif

      default:
        help();
        return -1;
    }
  }

  if(cellsize_check)
  {
    if(verbose)
      fprintf(stderr, "calculating character cell size\n");
    check_cell(font);
    get_slanting(font);

    fprintf(stderr, "character cell offsets:\n");
    fprintf(stderr, "top: %d, ", font->top);
    fprintf(stderr, "bottom: %d, ", font->bottom);
    fprintf(stderr, "left: %d, ", font->left);
    fprintf(stderr, "right: %d\n\n", font->right);
    fprintf(stderr, "slant size: %d, ", font->slant_size);
    fprintf(stderr, "italic correction: %d\n", font->slant_offset);
  }

  if(verbose > 1)
  {
    show_attributes(font);
    if(verbose > 2)
      show_widths(font);
  }

  if(output_raw)
  {
    idx = RangeEnd - RangeStart;
    a = font->header.height;
    if(verbose)
      fprintf(stderr, "output %d chars starting from %d as raw\n", idx, RangeStart);
    fwrite(font->map + RangeStart * a, sizeof(long), idx * a, stdout);
  }
  else
  {
    if(verbose < 2)
    {
      if(verbose)
	fprintf(stderr, "mapping font data back to stream\n");
      if(!map2stream(font))
	return -1;

      if(verbose)
	fprintf(stderr, "output font\n");
      save_font(font);
    }
  }
  return 0;
}
