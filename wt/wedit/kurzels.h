/*
 * Kurzel file load and use.
 *
 * Kurzel file format is following:
 *	# comment
 *	abbr	= expansion ~ text
 *		= another line
 *
 * Lines starting with alphanumeric characters are interpreted as
 * abbreviations.  The abbreviation text is everything right from the `='
 * character. White space between abbreviation and `=' is ignored so
 * that you can line up the explanation(s). If expansion extends to
 * multiple lines, those need to be prefixed with (optional white
 * space and) `=' characters too.
 *
 * If expansion contains `~' character(s), they are removed and cursor will
 * be positioned at the place of the last one.  On kurzel_t structure that
 * place is marked with `dx' and `dy' members which are offsets to the
 * position from the end of the expansion.  If you want `~' character(s) to
 * be seen on the expanded text, quote them with `\~'.
 *
 * Only spaces (' ') and tabs ('\t') are understood as white space.
 * Newlines ('\n') are interpreted as line breaks, but other control
 * characters are ignored (in case you got \r\n kurzel files).
 *
 * Lines which don't have `=' prefixed with appreviation and/or white
 * space are interpreted as comments. Comments will end an expansion.
 * For clarity it's better to start comment lines with same character,
 * for example `#'.
 *
 *
 * This file may be included into other programs as long as it remains
 * unmodified and I get an attribution for it somewhere into the program
 * documentation.
 *
 * (C) 1997 by Eero Tamminen
 */

typedef struct {
	char *abbr;		/* abbreviation */
	char *before;		/* expansion before cursor */
	char *after;		/* expansion after cursor */
} kurzel_t;


/* read and parser kurzel file, allocate, initialize and return kurzel array */
extern kurzel_t * read_kurzels(const char *file);

/* search abbreviation `string' of `count' lenght from `kurzel' array and
 * return pointer to the matched kurzel entry or NULL if none is found.
 */
extern kurzel_t * search_kurzel(kurzel_t *kurzel, const char *string, int count);

/* free kurzel array */
extern void free_kurzels(kurzel_t *kurzels);
