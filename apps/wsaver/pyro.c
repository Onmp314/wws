/*
 * pyro.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'pyrotechnics'
 *
 * CHANGES
 * ++eero, 6/98:
 * - added 'image' rockets (idea from 'flying_ggis' GGI demo).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <Wlib.h>
#include "wsaver.h"

#define BORDER	20

#define POINTS	128	/* max. rockets/particles flying at the same time */
#define DIVIDE	18	/* divide random rocket to about this many particles */

/* approx. 'ticks' before: */
#define NEXT	48	/* approx. time to next rocket */
#define BURN	40	/* approx. particles burn off time */

#define BITS	8	/* fixed point accuracy */


/* flags */
#define PARTICLE	1
#define ROCKET		2
#define MOVED		4

typedef struct {
	long x, y;
	short dx, dy;
	short type;
	short timer;
} point_t;


typedef struct {
	const char **map;
	short wd, ht, count;
} image_t;

static const char *image1[] = {
	" w     W",
	"W       W",
	"Ww  w  wW",
	"WwwW WwW",
	" Ww   w",
	NULL
};
static const char *image2[] = {
	"xxx xxx",
	" x   x",
	"  x x",
	"   x",
	"  x x",
	" x   x",
	"xxx xxx",
	NULL
};
static const char *image3[] = {
	"l   l l l l l l l",
	"l   l lll l l  l",
	"lll l l l ll  l l",
	NULL
};
static const char *image4[] = {
	"bbb   bbb bbb",
	"bb b bb   bb b",
	"bbb   bb  bb b",
	"bb b    b bb b",
	"bbbb bbbb bbb",
	NULL
};
static const char *image5[] = {
	"m   m m m  m mmmmm",
	"mm mm   mm m   m",
	"m m m m m mm   m",
	"m   m m m  m   m",
	NULL
};
static const char *image6[] = {
	" sss            ss   sss",
	"s    s  s sss  s  s s",
	"sss  s  s s  s s  s sss",
	"   s s  s s  s s  s    s",
	"sss   ss  s  s  ss  sss",
	NULL
};
static const char *image7[] = {
	" ggg    ggg    g",
	"gg     gg     gg",
	"gg gg  gg gg  gg",
	"gg  g  gg  g  gg",
	" ggg    ggg   g",
	NULL
};

static image_t image[] = {
	{ image1, 0, 0, 0 },
	{ image2, 0, 0, 0 },
	{ image3, 0, 0, 0 },
	{ image4, 0, 0, 0 },
	{ image5, 0, 0, 0 },
	{ image6, 0, 0, 0 },
	{ image7, 0, 0, 0 },
	{ NULL, 0, 0, 0 }
};
static int images;


static int init_images(void)
{
	const char **line, *c;
	int ht, wd, w, count;
	image_t *current = image;

	for (images = 0; (line = current->map); current++, images++) {

		count = ht = wd = 0;
		while ((c = *line++)) {

			w = 0;
			while (*c) {

				if (*c++ != ' ') {
					count++;
				}
				w++;
			}
			if (w > wd) {
				wd = w;
			}
			ht++;
		}
		current->count = count;
		current->wd = wd;
		current->ht = ht;
	}
	return images;
}


static inline void offset(long *x, long *y, int index, int max)
{
	/* sin(2*PI / 12 * index) * (1<<14) */
	static long sini[] = {
		0,  8192,  14189,  16384,  14189,  8192,
		0, -8192, -14189, -16384, -14189, -8192,
		0,  8192,  14189
	};
	*x = ((long)sini[index]   * max) >> 14;
	*y = ((long)sini[index+3] * max) >> 14;
}


static void explode(point_t *rocket,int *used, point_t *points)
{
	int burn, count, type;
	long x, y, dx, dy, pdx = 0, pdy = 0;
	const char **line = NULL, *c = NULL;
	image_t *current = NULL;
	point_t *particle;

	dx = rocket->dx;
	dy = rocket->dy;
	x = rocket->x - dx;
	y = rocket->y - dx;
	burn = BURN + random() % (BURN/2);

	type = random() % (images + 2);
	switch (type) {
	case 0:
		count = DIVIDE + random() % (DIVIDE/2);
		break;
	case 1:
		count = 18;
		break;
	default:
		current = &image[type-2];
		pdx -= current->wd << (BITS-3);
		pdy -= current->ht << (BITS-3);
		count = current->count;
		line = current->map;
		c = *line++;
		break;
	}

	if (*used + count > POINTS) {
		count = POINTS - *used;
		type = 0;
	}
	*used += count;

	while(--count >= 0) {

		particle = points++;
		particle->type = PARTICLE | MOVED;
		particle->timer = burn;

		switch (type) {

		case 0:
			/* randomized */
			pdx = random() % (2<<BITS) - (1<<BITS);
			pdy = random() % (2<<BITS) - (1<<BITS);
			break;

		case 1:
			/* rings */
			if (count >= 12) {
				/* inner ring: 6 */
				offset(&pdx, &pdy, (count%6)<<1, 1<<(BITS-1));
				particle->dx = dx + pdx;
				particle->dy = dy + pdy;
				particle->x = x;
				particle->y = y;
				particle->timer--;
				continue;
			}
			/* outer ring: 12 */
			offset(&pdx, &pdy, count, 1<<BITS);
			break;

		default:
			/* interpret image */
			for (;;) {
				if (!*c) {
					if (!line) {
						fprintf(stderr, "wsaver-pyro: encountered a bug!\n");
						w_exit();
						exit(-1);
					}
					pdx = -(current->wd << (BITS-3));
					pdy += 1<<(BITS-2);
					c = *line++;
					continue;
				}
				pdx += 1<<(BITS-2);
				if (*c++ == ' ') {
					continue;
				}
				break;
			}
			break;
		}

		particle->dx = dx + pdx;
		particle->dy = dy + pdy;
		particle->x = x + 2*pdx;
		particle->y = y + 2*pdy;
	}
}


static void shoot_rocket(point_t *point)
{
	int xsize = swidth - (2*BORDER);
	int ysize = sheight - BORDER;
	int value = random();

	point->type = ROCKET | MOVED;
	point->x = (random() % xsize + BORDER) << BITS;
	point->y = ysize << BITS;


	point->dx = ((xsize<<BITS)>>9) - random()%(xsize>>1) + (xsize>>2);
	if ((point->x >> (BITS-1)) > xsize+BORDER) {
		point->dx = -point->dx;
	}
	point->dy = value%(ysize>>1) - (ysize>>2) - ((ysize<<BITS)>>7);

	point->timer = ((ysize>>1) + value % (ysize>>1)) >> 1;
}


void save_pyro(void)
{
	/* used is index to first free point */
	static point_t *points;
	point_t tmp, *current;
	int idx, used = 0, timer = 1;
	short oldx, oldy;
	WEVENT *ev;

	if (!points) {
		if (!init_images()) {
			fprintf(stderr, "wsaver-pyro: images missing!\n");
			w_exit();
			exit(-1);
		}
		if (!(points = malloc(POINTS*sizeof(point_t)))) {
			fprintf(stderr, "wsaver: malloc failed!\n");
			w_exit();
			exit(-1);
		}
	}

	for (;;) {

		/* clear rockets/particles from black bg */
		w_setmode(win, M_DRAW);

		idx = 0;
		while (idx < used) {

			current = &points[idx];
			oldx = current->x >> BITS;
			oldy = current->y >> BITS;

			current->x += current->dx;
			current->y += current->dy;

			/* timer / gravity */
			current->timer -= 1;
			current->dy += 1<<(BITS-6);

			if ((current->x >> BITS) != oldx ||
			    (current->y >> BITS) != oldy ||
			    !current->timer) {

				/* w_pbox(win, oldx-1, oldy-1, 3, 3); */
				w_pcircle(win, oldx, oldy, 2);

				if (current->timer) {
					current->type |= MOVED;

					/* delete if particle out of screen */
					if ((current->type & PARTICLE) &&
					    (oldx < 0 || oldx > swidth ||
					     oldy > sheight)) {
						*current = points[--used];
					} else {
						idx++;
					}
					continue;
				}
				/* timer expired */

				/* move latest to current */
				tmp = *current;
				*current = points[--used];
				
				if (tmp.type & ROCKET) {
					/* create new points */
					explode(&tmp, &used, &points[used]);
				}
				continue;
			}
			idx++;
		}

		if (!--timer) {
			timer = random() % NEXT + NEXT/2;
			if (used < POINTS) {
				shoot_rocket(&points[used]);
				used++;
			}
		}

		/* redraw rockets/particles */
		w_setmode(win, M_CLEAR);

		current = &points[used];
		while (--current >= points) {

			if (current->type & MOVED) {
				oldx = current->x >> BITS;
				oldy = current->y >> BITS;
				/* w_pbox(win, oldx-1, oldy-1, 3, 3); */
				w_pcircle(win, oldx, oldy, 2);
				current->type &= ~MOVED;
			}
		}

		if ((ev = w_queryevent(NULL, NULL, NULL, timeout))) {
			switch (ev->type) {

				case EVENT_GADGET:
					if (ev->key == GADGET_EXIT) {
						w_exit();
						exit(0);
					}
					break;

				default:
					return;
			}
		}
	}
}
