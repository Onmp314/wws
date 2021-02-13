/* 
 * some toolkit utility functions which don't deal with the widgets
 * itself:
 *	read_config(), wt_variable()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"


/* max. configuration file line lenght */
#define MAX_LINELEN	256

/* list item containing a (variable,value) pair from the W toolkit user
 * configuration file.
 */
typedef struct _wt_variable_t {
	struct _wt_variable_t *next;
	char *variable;
	char *value;
} wt_variable_t;


/* configuration variable / value pairs */
static wt_variable_t *Variables;


/* search the variable list for given variable and return it's value
 * string or NULL
 */
const char *
wt_variable(const char *id)
{
	wt_variable_t *ptr = Variables;

	while(ptr) {
		if (!strcmp(id, ptr->variable)) {
			return ptr->value;
		}
		ptr = ptr->next;
	}
	return NULL;
	
}

/* read given configuration file from $HOME dir, prefixed with dot.
 *
 * search and allocate a list of configuration (variable, value) pairs and
 * set wt_global options according to them or link them to Variables list.
 */
void read_config(const char *config)
{
	char *home, *file, *line, *value, c;
	int idx, maxlen, eq;
	wt_variable_t *ptr;
	FILE *fp;

	if (!(home = getenv("HOME"))) {
		fprintf(stderr, "Wt read_config(): $HOME not set\n");
		return;
	}
	maxlen = strlen(home) + strlen(config) + 3;
	if (maxlen < MAX_LINELEN) {
		maxlen = MAX_LINELEN;
	}
	if (!(file = malloc(maxlen))) {
		return;
	}
	sprintf(file, "%s/.%s", home, config);
	if(!(fp = fopen(file, "r"))) {
		/* no configuration file */
		free(file);
		return;
	}

	/* read configuration variables */
	while (fgets(file, maxlen, fp)) {

		line = file;
		/* search variable start */
		while(*line && *line <= 32) {
			line++;
		}

		eq = 0;
		for (idx = 0; idx < maxlen && line[idx]; idx++) {
			c = line[idx];
			/* search value end */
			if ((c < 32 && c != '\t') || c == '#') {
				break;
			}
			if (c == '=') {
				/* variable end / value start */
				eq = idx;
			}
		}
		if (!eq) {
			continue;
		}

		/* remove white space at the end and start of the value */
		while(--idx > eq && line[idx] < 32)
			;
		line[++idx] = 0;
		value = line + eq;
		while(*++value && *value <= 32)
			;

		/* remove white space at the end of variable name */
		while(line[--eq] <= 32)
			;
		line[++eq] = 0;

		idx = (line + idx) - value;
		/* 'eq' got now variable and 'idx' value lenght */


		/* check for global defaults */

		if (!strcmp(line, "font_normal")) {
			wt_global.font_normal = strdup(value);
			continue;
		}
		if (!strcmp(line, "font_fixed")) {
			wt_global.font_fixed = strdup(value);
			continue;
		}
		if (!strcmp(line, "font_size")) {
			wt_global.font_size = atoi(value);
			continue;
		}

		/* link into variables list */

		ptr = malloc(sizeof(wt_variable_t)  + eq + idx + 2);
		ptr->variable = (char *)ptr + sizeof(wt_variable_t);
		ptr->value = ptr->variable + eq + 1;

		strcpy(ptr->variable, line);
		strcpy(ptr->value, value);

		ptr->next = Variables;
		Variables = ptr;
	}
	fclose(fp);
	free(file);
}


/* 
 * I added this function (which terminates the program if even W toolkit
 * default font couldn't be loaded) as people may easily set fonts which
 * don't exist and programs not checking widget creation can then
 * 'mysteriously' chrash...
 */
WFONT *wt_loadfont(const char *fname, int fsize, int flags, int fixed)
{
	WFONT *fp;

	if (!fname || !*fname) {
		if (fixed) {
			fname = wt_global.font_fixed;
		} else {
			fname = wt_global.font_normal;
		}
	}
	if (!fsize) {
		fsize = wt_global.font_size;
	}
	if ((fp = w_loadfont(fname, fsize, flags))) {
		return fp;
	}
	if ((fname == wt_global.font_normal ||
	     fname == wt_global.font_fixed) &&
	    fsize == wt_global.font_size && !flags) {
		fprintf(stderr, "Wt: unable to load Wt default font %s%d!\n", fname, fsize);
		exit(-1);
	}
	return NULL;
}
