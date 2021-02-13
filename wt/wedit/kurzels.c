/*
 * Kurzel file load and use.
 *
 * Kurzel file format is following (see header file for more information):
 *	# comment, `~' is cursor place
 *	abbr	= expansion ~ text
 *		= another line
 *
 * This file may be included into other programs as long as it remains
 * unmodified and I get an attribution for it somewhere into the program
 * documentation.
 *
 * (C) 1997 by Eero Tamminen
 */

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kurzels.h"


/* parser string containing abbreviations into a kurzel array */
static kurzel_t *
parse_kurzels(char *data)
{
	char *dptr, *sptr, *abbr;
	int dx, dy, index, count, lenght, lines;
	kurzel_t *kurzels;

	/* count kurzels and char space needed */
	count = lenght = 0;
	sptr = data;
	while(*sptr) {
		if (isalnum(*sptr)) {
			index = 1;
			/* count abbreviation */
			while (isalnum(*++sptr)) {
				index++;
			}
			for(lines = 0;; lines++) {
				/* skip white space on _this_ line */
				while (*sptr && *sptr <= ' ' && *sptr != '\n') {
					sptr++;
				}
				/* not or end of abbreviation? */
				if (*sptr != '=') {
					break;
				}
				/* skip and count expansion */
				while (*++sptr && *sptr != '\n') {
					/* ignore control codes */
					if (*sptr < ' ' && *sptr != '\t') {
						continue;
					}
					index++;
				}
				if (*sptr == '\n') {
					index++;
					sptr++;
				}
			}
			/* valid abbreviation? */
			if (lines) {
				if (*(sptr-1) == '\n') {
					index--;
				}
				/* abbreviation and expansion strings */
				lenght += index + 2;
				count++;
			}
			/* no commentary? */
			if (isalnum(*sptr)) {
				continue;
			}
		}
		/* pass commentary */
		while (*sptr) {
			if (*sptr++ == '\n')
			break;
		}
	}

	/* allocate space: structs + NULL one + strings */
	kurzels = malloc(++count * sizeof(kurzel_t) + lenght);
	if (!kurzels) {
		return NULL;
	}
	dptr = (char *)(kurzels + count);

	/* copy kurzels */
	dx = dy = count = lenght = 0;
	sptr = data;
	while(*sptr) {
		if (isalnum(*sptr)) {
			index = 1;
			abbr = sptr;
			/* count abbreviation */
			while (isalnum(*++sptr)) {
				index++;
			}
			for(lines = 0;; lines++) {
				/* skip white space on _this_ line */
				while (*sptr && *sptr <= ' ' && *sptr != '\n') {
					sptr++;
				}
				/* not or end of abbreviation? */
				if (*sptr != '=') {
					break;
				}
				if (!lines) {
					/* set string pointers */
					kurzels[count].abbr = dptr;
					memcpy(dptr, abbr, index);
					dptr[index] = '\0';
					dptr += index + 1;
					kurzels[count].before = dptr;
					dx = 0;
					dy = 0;
				}
				index = 0;
				/* copy one expansion line */
				while (*++sptr && *sptr != '\n') {
					/* ignore control codes */
					if (*sptr < ' ' && *sptr != '\t') {
						continue;
					}
					/* position char? */
					if (*sptr == '~') {
						/* quoted... */
						if (*(sptr-1) == '\\') {
							*(dptr-1) = '~';
						} else {
							*dptr++ = '\0';
							kurzels[count].after = dptr;
						}
					} else {
						*dptr++ = *sptr;
						index++;
					}
				}
				if (*sptr == '\n') {
					*dptr++ = *sptr++;
				}
			}
			/* valid abbreviation? */
			if (lines) {
				/* no newline on last line */
				if (*(dptr-1) == '\n') {
					dptr--;
				}
				*dptr++ = '\0';
				count++;
			}
			/* no commentary? */
			if (isalnum(*sptr)) {
				continue;
			}
		}
		/* pass commentary */
		while (*sptr) {
			if (*sptr++ == '\n')
			break;
		}
	}
	memset(&kurzels[count], 0, sizeof(kurzel_t));
	return kurzels;
}


/* read and parser kurzel file, return kurzel array */
kurzel_t *
read_kurzels(const char *file)
{
	kurzel_t *kurzels;
	struct stat st;
	char *data;
	FILE *fp;

	if ((fp = fopen(file, "rb"))) {

		stat(file, &st);
		if((data = malloc(st.st_size+2)))
		{
			fread(data, st.st_size, 1, fp);
			data[st.st_size+1] = 0;
			data[st.st_size] = 0;
			kurzels = parse_kurzels(data);
			free(data);
		} else {
			return NULL;
		}
		fclose(fp);
		return kurzels;
	}
	return NULL;
}


/* search abbreviation `string' of `count' lenght from `kurzel' array, set
 * cursor offsets to `dx' and `dy' and return abbreviation expansion.
 */
kurzel_t *
search_kurzel(kurzel_t *kurzel, const char *string, int count)
{
	char *abbr;
	int index;

	if (!kurzel) {
		return NULL;
	}
	for(; kurzel->abbr; kurzel++)
	{
		index = 0;
		abbr = kurzel->abbr;
		while(index < count && string[index] == *abbr)
		{
			abbr++;
			index++;
		}
		if(index == count)
			break;
	}
	if(kurzel->abbr) {
		return kurzel;
	}
	return NULL;
}


/* free kurzel array */
void
free_kurzels(kurzel_t *kurzels)
{
	if (kurzels) {
		free(kurzels);
	}
}
