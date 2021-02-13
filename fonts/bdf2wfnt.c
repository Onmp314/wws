/* 
 * bdf2wfnt.c (w) 1998 by Eero Tamminen
 * 
 * An X BDF font description to W font conversion tool.
 *
 * NOTES:
 * - Font size is according to it's bounding box unless '-m' option is used.
 * - BDF 'black' & 'bold' weights are mapped to F_BOLD and italic' &
 *   'oblique' styles are mapped to F_ITALIC.  No other styles are mapped.
 * - Font copyright and notice information are saved to font info.
 * - X font family name may exceed the space in W font header.  if it does,
 *   the name is truncated.  ATM bdf2wfnt preserves the case of name.
 * - Characters between codes 32-126 are identified according to their X
 *   name, rest of the chars are identified by their ENCODE line, if it
 *   follows immedietly the STARTCHAR line and has a legal value (1-255).
 *
 * Based on heavily modified and expanded bdf2c.c source Copyright (c) 1994
 * by Andrew Scherbier. See file 'COPYRIGHT'.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "../server/types.h"	/* font file header */
#include "Wlib.h"		/* font flags */


#define BUFFER_SIZE	256	/* for bdf input lines */


#define MAGIC	0x57464e31	/* W font file ID */
typedef struct
{
  FONTHDR header;
  char *info;			/* font info (author etc.) */
  uchar widths[256];		/* induvidual character widths */
  ulong *data;			/* long aligned character bitstream */
  ulong *map;			/* font bitmaps */
} MEMFONT;

static int verbose, minimal;	/* options */
static int ascent, descent;	/* additional info */


/* X character names */
#define START_INDEX	32
static char *charmap[] = {
	"space",
	"exclam",
	"quotedbl",
	"numbersign",
	"dollar",
	"percent",
	"ampersand",
	"quoteright",
	"parenleft",
	"parenright",
	"asterisk",
	"plus",
	"comma",
	"minus",
	"period",
	"slash",
	"zero",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"colon",
	"semicolon",
	"less",
	"equal",
	"greater",
	"question",
	"at",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"bracketleft",
	"backslash",
	"bracketright",
	"asciicircum",
	"underscore",
	"quoteleft",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"braceleft",
	"bar",
	"braceright",
	"asciitilde",
	NULL
};


/* 
 * read character from BDF file.  return 1 for success, 0 for failure.
 */
static int convert_char(MEMFONT *font, char *name, FILE *fp)
{
	static char buffer[BUFFER_SIZE];

	/* character width/height, bitmap width/height, offsets to baseline */
	int width, height, wd, ht, yoff, xoff;
	int idx, read_data, line, count;
	ulong value, *data;
	char *ptr;

	/*
	 * Lookup the character index
	 */
	for (idx = 0; charmap[idx]; idx++) {
		if (strcmp(name, charmap[idx]) == 0)
			break;
	}

	if (charmap[idx]) {
		idx += START_INDEX;
	} else {
		/* unrecogniced name, try to get encoding index */
		if (fgets(buffer, sizeof(buffer), fp) &&
		    strncmp(buffer, "ENCODING", 8) == 0) {
			sscanf(buffer+9, "%d", &idx);
			if (idx <= 0 || idx > 255) {
				return 0;		/* Not found */
			}
		}
	}

	height = font->header.height;
	data = font->map + idx * height;
	read_data = line = width = wd = ht = yoff = xoff = 0;

	if (verbose) {
		fprintf(stderr, "character: '%c' (%d)\n", idx, idx);
	}

	while (fgets(buffer, sizeof(buffer), fp)) {

		/*
		 * Read BITMAP data
		 */
		if (read_data) {
			if (strncmp(buffer, "ENDCHAR", 7) == 0) {
				return 1;
			}
			if (line++ >= height) {
				fprintf(stderr, "BBX <-> BITMAP inconsistency\n");
				return 1;
			}

			ptr = buffer;
			if (!isxdigit(*ptr)) {
				continue;
			}
			count = 0;
			value = 0;
			for (;;) {
				if (isdigit(*ptr)) {
					value |= (*ptr - '0') & 15;
				} else {
					value |= (*ptr - 'A' + 10) & 15;
				}
				count += 4;
				ptr++;
				if (!isxdigit(*ptr)) {
					break;
				}
				value <<= 4;
			}
			if ((count = count - wd) > 0) {
				value >>= count;
			}
			/* not an 'italic correction'? */
			if (xoff > 0) {
				value <<= xoff;
			}
			*data++ = value;
			continue;
		}

		/*
		 * Search for the BITMAP line
		 */
		if (strncmp(buffer, "BITMAP", 6) == 0) {
			if (verbose) {
				fprintf(stderr,
					"wd: %d, ht: %d, xoff: %d, yoff: %d, width: %d\n",
					wd, ht, xoff, yoff, width);
			}
			if (width > wd) {
				/* don't know whether these help anything... */
				if (xoff > 0) {
					if (minimal) {
						width -= xoff;
					}
					xoff = width - wd - xoff;
				} else {
					xoff = width - wd + xoff;
				}
			} else {
				width = wd;
			}
			if (width) {
				font->widths[idx] = width;
			} else {
				fprintf(stderr, "char has no width!\n");
				return 0;
			}
			line = height + descent - ht - yoff;

			if (line < 0 || line + ht > height) {
				fprintf(stderr, "ascent/descent error\n");
				if (line > 0) {
					line = height - ht;
				} else {
					line = 0;
				}
			}
			data += line;
			read_data = 1;

		} else if (strncmp(buffer, "BBX", 3) == 0) {
			sscanf(buffer+4, "%d %d %d %d", &wd, &ht, &xoff, &yoff);

		} else if (strncmp(buffer, "DWIDTH", 6) == 0) {
			sscanf(buffer+7, "%d", &width);
		}
	}

	fprintf(stderr, "abrupt bdf file end!\n");
	return 0;
}


static char *skip_space(char *token)
{
	while (*token && *token <= ' ') {
		token++;
	}
	return token;
}

static char *add_info(char *old, char *new)
{
	size_t len;
	char *info;

	if (!new) {
		return old;
	}
	if (!old) {
		return strdup(new);
	}
	free(old);
	len = strlen(old);
	info = malloc(len + strlen(new) + 2);
	if (!info) {
		fprintf(stderr, "allocating space for '%s' + '%s' failed\n",
			old, new);
		return NULL;
	}
	strcpy(info, old);
	info[len++] = ' ';
	strcpy(info+len, new);
	return info;
}

/*
 * return number of parsed characters. Types pointed by args may all
 * be modified.
 */
static int parse_bdf(MEMFONT *font, FILE *fp, char *buffer)
{
	FONTHDR *header = &(font->header);
	char *token;

	token = strtok(buffer, " \r\n");

	/* character info / bitmap */
	if (strcmp(token, "STARTCHAR") == 0) {
		if (!font->map) {
			long size = header->height;

			if (!size) {
				fprintf(stderr, "font has no height!\n");
				exit(-1);
			}
			/* with the height we can allocate bitmaps */
			font->map = calloc(256, size * sizeof(long));
		}
		token = strtok(NULL, " \r\n");
		return convert_char(font, token, fp);
	}

	/* font size */
	if (strcmp(token, "FONTBOUNDINGBOX") == 0) {
		if (font->map) {
			return 0;
		}
		token = strtok(NULL, "\r\n");
		sscanf (token, "%hd %hd %d %d",
			&(header->maxwidth), &(header->height),
			&ascent, &descent);

		if (minimal) {
			/* most characters should fit this quite OK too */
			header->height += descent;
		}
		header->baseline = header->height + descent;

	/* font family */
	} else if (strcmp(token, "FAMILY_NAME") == 0) {
		token = strtok(NULL, " \"\r\n");
		strncpy(header->family, token, sizeof(header->family));

	/* font styles */
	} else if (strcmp(token, "WEIGHT_NAME") == 0) {
		token = strtok(NULL, " \"\r\n");
		if (strcasecmp(token, "black") == 0 ||
		    strcasecmp(token, "bold") == 0) {
			header->styles |= F_BOLD;
		}
	} else if (strcmp(token, "SLANT") == 0) {
		token = strtok(NULL, " \"\r\n");
		/* IMHO X Italic and Oblique can both be mapped to W italic */
		if (*token == 'i' || *token == 'o' ||
		    *token == 'I' || *token == 'O') {
			header->styles |= F_ITALIC;
		}

	/* font info <- copyright / notices */
	} else if (strcmp(token, "COPYRIGHT") == 0 ||
	           strcmp(token, "NOTICE") == 0) {
		/* not sure if this works with all strtok()
		 * versions...
		 */
		token = skip_space(token + strlen(token) + 1);
		token = strtok(token, "\"\r\n");
		if (*token) {
			font->info = add_info(font->info, token);
		}
	}

	return 0;
}


/* 
 * W font saving utility functions
 */

/* set font maxwidth and flags according to character widths */
static void check_widths(MEMFONT *font)
{
	int idx, width, start = 32, end = 127, max = 0;

	for (idx = 0; idx < 32; idx++) {
		if (font->widths[idx]) {
			start = idx;
			break;
		}
	}
	for (idx = 255; idx > 127; idx--) {
		if (font->widths[idx]) {
			end = idx;
		}
	}

	font->header.flags &= ~(F_ASCII | F_LATIN1 | F_SYMBOL | F_PROP);

	if (start < 32) {
		/* symbols, native DOS, Mac, Atari... fonts */
		font->header.flags |= F_SYMBOL;
	} else {
		if (end > 127) {
			font->header.flags |= F_LATIN1;
		} else {
			font->header.flags |= F_ASCII;
		}
	}

	for(idx = start; idx <= end; idx++) {
		width = font->widths[idx];
		if(width > max) {
			if(max) {
				font->header.flags |= F_PROP;
			}
			max = width;
		}
	}
	font->header.maxwidth = max;
}

static ulong *map2stream(MEMFONT *font)
{
	register ulong *cptr, cbit, cdata, *dptr, dbit, ddata;
	register short cheight, lcwidth;
	short cwidth, height;
	ushort idx;
	long longs;

	if (font->data) {
		free(font->data);
	}
	longs = 0;
	height = font->header.height;
	for (idx = 0; idx < 256; idx++) {
		longs += (font->widths[idx] * height + 31) >> 5;
	}
	longs = longs * sizeof(long);
	font->data = malloc(longs);
	font->header.lenght = longs;

	if (!(font->map && font->data)) {
		fprintf(stderr, "font bitmap streaming failed!\n");
		return NULL;
	}

	memset(font->data, 0, longs);
	dptr = font->map;
	cptr = font->data;
	cbit = 0;

	for (idx = 0; idx < 256; idx++) {
		cwidth = font->widths[idx];
		if (!cwidth) {
			dptr += height;
			continue;
		}

		cdata = 0;
		cbit = 0x80000000;
		cheight = height;
		while (--cheight >= 0) {
			dbit = 1 << (cwidth-1);
			ddata = *dptr;
			lcwidth = cwidth;
			while (lcwidth-- > 0) {
				if (ddata & dbit) {
					cdata |= cbit;
				}
				dbit >>= 1;
				if (!(cbit >>= 1)) {
					cbit = 0x80000000;
					*cptr++ = cdata;
					cdata = 0;
				}
			}
			dptr++;
		}
		if (cbit != 0x80000000) {
			*cptr++ = cdata;
		}
	}
	free(font->map);
	font->map = NULL;
	return font->data;
}

/* Convert header variable byte orders between host<->big endian. */
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
	header->thicken     = ntohs(header->thicken);
	header->skew        = ntohs(header->skew);
	header->effect_mask = ntohs(header->effect_mask);

	/* font character cell size */
	header->baseline = ntohs(header->baseline);
	header->maxwidth = ntohs(header->maxwidth);
}

static void save_font(MEMFONT *font)
{
	long idx, longs, info_len = 0;
	FONTHDR *header = &font->header;

	if (font->info && *(font->info)) {
		info_len = strlen(font->info) + 1;
	}

	/* set default values */
	header->magic = MAGIC;
	header->header = sizeof(FONTHDR) + info_len;
	header->effect_mask = ~header->styles & F_STYLEMASK;
	header->thicken = 1;
	header->skew = 0xAAAA;

	/* sets header->flags */
	check_widths(font);

	/* convert font data to bitstream, sets header->lenght */
	if (!map2stream(font)) {
		fprintf(stderr, "font bitstreaming failed!\n");
		return;
	}
	longs = font->header.lenght / sizeof(long);

	/* save header... */
	convert_header(&font->header);
	fwrite(&font->header, 1, sizeof(FONTHDR), stdout);
	if (info_len) {
		fwrite(font->info, 1, info_len, stdout);
	}
	fwrite(font->widths, 1, 256, stdout);

	/* ...and data */
	for (idx = 0; idx < longs; idx++) {
		font->data[idx] = htonl(font->data[idx]);
	}
	idx = fwrite(font->data, sizeof(long), longs, stdout);
	if (idx != longs) {
		fprintf(stderr, "font save failed!\n");
	}
}


/*
 * Get BDF font info from file and call above functions accordingly...
 */

static void usage(void)
{
	fprintf(stderr, "\
usage: bdf2wfnt [options] [BDF file]  >  <W font file>\n\
options:\n\
	-v  report conversions\n\
	-m  minimal size\n\
	-h  this help\n\
if there's no BDF file, stdin is read instead.\n");
}

int main(int argc, char **argv)
{
	static MEMFONT font;
	static char buffer[BUFFER_SIZE];
	int characters = 0, index = 0;
	FILE *fp;

	while(++index < argc && argv[index][0] == '-' && !argv[index][2]) {
		switch (argv[index][1]) {
			case 'm':
				minimal = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				usage();
				return -1;
		}
	}

	if (index < argc) {
		if (!(fp = fopen(argv[index], "rb"))) {
			fprintf(stderr, "no bdf file!\n");
			usage();
			return -1;
		}
	} else {
		fp = stdin;
	}

	/* get attributes, character bitmaps etc. */
	while (fgets(buffer, sizeof(buffer), fp)) {
		characters += parse_bdf(&font, fp, buffer);
	}

	if (verbose) {
		fprintf(stderr,	"font: '%s'\n", argv[index]);
		fprintf(stderr,	"wd: %d, ht: %d, ascent: %d, descent: %d\n",
			font.header.maxwidth, font.header.height,
			ascent, descent);
		}

	if (characters) {
		save_font(&font);
	} else {
		fprintf(stderr, "no characters converted!\n");
	}
	return 0;
}

