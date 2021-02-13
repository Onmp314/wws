/*
 *  W Window System display code for Heretic, (w) 1999 by Eero Tamminen
 * 
 * INSTALLATION
 * - Put this to heretic 'graphics' subdirectory.
 * - Add 'wws' target to Heretic Makefile so that this is
 *   the graphics object and libs line is '-lW'.
 * - 'make wws' and install Heretic as usual.
 * 
 * RUNNING
 * - On W servers which don't support special key mapping to W key 'codes',
 *   you can use the number keypad (numlock on) for moving in menus.
 * - On W servers which don't support key release events, you can
 *   use the mouse instead of keyboard.  To be able to move with
 *   mouse instead of just looking around, you should disable the
 *   'mouselook' from game options dialog!
 * - Use SPACE to 'calibrate' mouse.  It will set the current mouse
 *   position as center ie player doesn't move when mouse is in that
 *   position.
 * - If mouse input disturbs your gaming, use new '-nomouse' Heretic
 *   argument, and mouse moving won't anymore move your character.
 *
 * OTHER NOTES
 * - Monochrome support in addition to 8-bit.
 * - Based on the Heretic libGGI framework.
 */

/* needed only for scaling, only when on x86 and you don't have glibc */
/* #define LITTLE_ENDIAN */

#include <stdlib.h>
#include <ctype.h>
#include <Wlib.h>

#include "doomdef.h"

/* show FPS counter */
#define SHOW_FPS

static WWIN	*wvis = NULL;

static int	xmouseoff = 0;
static int	ymouseoff = 0;

static int	realwidth, realheight, scale;
static uchar	*palette;		/* palette mapping */
static BITMAP	*frame;

/* slower but nicer looking than default ordered dithering... */
#define FSDITHER_MONO

static int	makemono = 0;
static uchar	grayscale[256];		/* monochrome mapping */


#ifdef SHOW_FPS
#include <unistd.h>
#include <sys/time.h>

static struct timeval	starttime;
static long		totalframes;
static int		showfps = 0;
static WFONT		*font;
#endif


/* just doubles the image size */
static inline void
do_scale8(int xsize, int ysize, uchar *dest, uchar *src)
{
	int i, j, stride = frame->upl * frame->unitsize;
	int destinc = stride*2-xsize*2;
	uchar *palptr = palette;

	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; /* i is incremented below */) {
			register ulong pix1 = palptr[src[i++]];
			register ulong pix2 = palptr[src[i++]];
#ifdef LITTLE_ENDIAN
			*((ulong *) (dest + stride))
				= *((ulong *) dest)
				= (pix1 | (pix1 << 8)
				   | (pix2 << 16) | (pix2 << 24));
#else
			*((ulong *) (dest + stride))
				= *((ulong *) dest)
				= (pix2 | (pix2 << 8)
				   | (pix1 << 16) | (pix1 << 24));
#endif
			dest += 4;
		}
		dest += destinc;
		src += xsize;
	}
}

static inline void
do_copy8(int xsize, int ysize, uchar *dest, uchar *src)
{
	int i, j, stride = frame->upl * frame->unitsize;
	uchar *palptr = palette;
	
	for (j = 0; j < ysize; j++) {
		for (i = 0; i < xsize; i++) {
			dest[i] = palptr[src[i]];
		}
		dest += stride;
		src += xsize;
	}
}

void I_UpdateNoBlit (void)
{
}

void I_FinishUpdate (void)
{
	int in_place = 1;
	BITMAP *bm, bm2;
	char *error;

	if (scale) {
		do_scale8(screenwidth, screenheight,
			frame->data, screen);
	} else {
		do_copy8(screenwidth, screenheight,
			frame->data, screen);
	}

#ifdef FSDITHER_MONO
	if (makemono) {
		/* copy correct bitmap header to modifiable backup */
		bm2 = *frame;

		/* FS-dither to monochrome *in-place* (changes header) */
		if (!fs_direct2mono(&bm2, in_place, &error)) {
			I_Error(error);
		}
		bm = &bm2;

		/* otherwise w_putblock() will use not-in-place
		 * ordered dithering for the monochrome server.
		 * It doesn't look as good but is slightly faster.
		 */
	} else
#endif
	{
		bm = frame;
	}
	/* inefficient: we always update the whole window... */
	w_putblock(bm, wvis, 0, 0);

#ifdef SHOW_FPS
	/* show the frame counter */
	if (showfps) {
		struct timeval curtime;
		char str[64];
		float diff;
		
		totalframes++;
		gettimeofday(&curtime, NULL);
		diff = (curtime.tv_sec - starttime.tv_sec);
		diff += ((float)curtime.tv_usec - starttime.tv_usec) / 1000000.0;
		if (diff) {
			sprintf(str, "FPS: %.1f", totalframes / diff);
			w_printstring(wvis, 0, 0, str);
		}
	}
#endif
}

void I_ShutdownGraphics(void)
{
	if (wvis != NULL) {
		if (screen) {
			free(screen);
			screen = NULL;
		}
		if (palette) {
			free(palette);
			palette = NULL;
		}
		w_close(wvis);
		wvis = NULL;
	}
	w_exit();
}


void I_StartFrame (void)
{
}

int key(WEVENT *ev)
{
	long label = ev->key;

	/* remove left/right info */
	label ^= WMOD_SIDE(label);

	switch(label) {

		/* calibrate mouse position */
	case ' ':
		xmouseoff = 0;
		ymouseoff = 0;
		break;

		/* run */
	case WMOD_SHIFT:
		return KEY_RSHIFT;
		/* strafe */
	case WMOD_ALT:
	case WMOD_ALTGR:
	case WMOD_META:
		return KEY_RALT;
		/* fire */
	case WMOD_CTRL:
	case '5':
		return KEY_RCTRL;

	case '\f':
	case '\r':
		return KEY_ENTER;
	case '\b':
		return KEY_BACKSPACE;
	case '\e':
		return KEY_ESCAPE;
	case '\t':
		return KEY_TAB;
	case 'p':
		return KEY_PAUSE;

	/* in addition to 'special keys',
	 * numberpad keys can also be used
	 */
	case WKEY_DEL:
	case '.':
		return KEY_DELETE;
	case WKEY_INS:
	case '0':
		return KEY_INSERT;
	case WKEY_PGUP:
	case '9':
		return KEY_PAGEUP;
	case WKEY_PGDOWN:
	case '3':
		return KEY_PAGEDOWN;
	case WKEY_HOME:
	case '7':
		return KEY_HOME;
	case WKEY_END:
	case '1':
		return KEY_END;
	case WKEY_UP:
	case '8':
		return KEY_UPARROW;
	case WKEY_DOWN:
	case '2':
		return KEY_DOWNARROW;
	case WKEY_LEFT:
	case '4':
		return KEY_LEFTARROW;
	case WKEY_RIGHT:
	case '6':
		return KEY_RIGHTARROW;

	case WKEY_F1:
		return KEY_F1;
	case WKEY_F2:
		return KEY_F2;
	case WKEY_F3:
		return KEY_F3;
	case WKEY_F4:
		return KEY_F4;
	case WKEY_F5:
		return KEY_F5;
	case WKEY_F6:
		return KEY_F6;
	case WKEY_F7:
		return KEY_F7;
	case WKEY_F8:
		return KEY_F8;
	case WKEY_F9:
		return KEY_F9;
	case WKEY_F10:
		return KEY_F10;
	case WKEY_F11:
		return KEY_F11;
	case WKEY_F12:
		return KEY_F12;

	/* Some special cases */
	case '+':
		return KEY_EQUALS;
	case '-':
		return KEY_MINUS;

	default:
		if ((label >= '0' && label <= '9') ||
		    (label >= 'A' && label <= 'Z') ||
		    label == '.' ||
		    label == ',') {
			/* We want lowercase */
			return tolower(label);
		}
		if (label < 256) {
			/* ASCII key - we want those */
			return label;
		}
	}
	return 0;
}

void I_GetEvent(void)
{
	static int buttonstate;
	event_t event;
	WEVENT *ev;
	
	while (1) {
		event.data2 = event.data3 = 0;

		if (!(ev = w_queryevent(NULL, NULL, NULL, 0))) {
			/* 8x8 pixel non-move area in the middle */
			if (xmouseoff > 4) {
				event.data2 = xmouseoff - 4;
			} else if (xmouseoff < -4) {
				event.data2 = xmouseoff + 4;
			}
			if (ymouseoff > 4) {
				event.data3 = ymouseoff - 4;
			} else if (ymouseoff < -4) {
				event.data3 = ymouseoff + 4;
			}
			/* are we 'on the move'? */
			if (event.data2 || event.data3) {
				/* turn 4 times as fast as move */
				event.data2 <<= 1;
				event.data3 >>= 1;
				event.data1 = buttonstate;
				event.type = ev_mouse;
				D_PostEvent(&event);
			}
			return;
		}

		switch (ev->type)
		{
		case EVENT_KEY:
			event.data1 = key(ev);
			event.type = ev_keydown;
			D_PostEvent(&event);
#ifdef SHOW_FPS
			if (event.data1 == KEY_BACKSPACE &&
			    gamestate == GS_LEVEL) {
				/* Toggle and reset the FPS counter */
				gettimeofday(&starttime, NULL);
				showfps = !showfps;
				totalframes = 0;
			}
#endif
			break;

		case EVENT_KRELEASE:
			event.data1 = key(ev);
			event.type = ev_keyup;
			D_PostEvent(&event);
			break;

			
		case EVENT_MPRESS:
			if (ev->key == BUTTON_LEFT) {
				buttonstate = 1;
				event.data1 = buttonstate;
				event.type = ev_mouse;
				D_PostEvent(&event);
			}
			break;
			
		case EVENT_MRELEASE:
			if (ev->key == BUTTON_LEFT) {
				buttonstate = 0;
				event.data1 = buttonstate;
				event.type = ev_mouse;
				D_PostEvent(&event);
			}
			break;

		case EVENT_MMOVE:
			xmouseoff += ev->x;
			ymouseoff -= ev->y;
			break;
			
		case EVENT_GADGET:
			I_Error("External quit\n");
		}
	}
}

void I_StartTic (void)
{
	if (wvis) I_GetEvent();
}

void I_SetPalette (byte* pal)
{
	int i;
	long value;
	static rgb_t rgb[256];
	byte *gamma = gammatable[usegamma];

	if (palette) {
#if 0
		/* seems to set bogus color values and works without... */
		for (i = 0; i < 256; i++) {
			n.red   = gamma[*pal++];
			n.green = gamma[*pal++];
			n.blue  = gamma[*pal++];
			w_changeColor(wvis, palette[i], n.red, n.green, n.blue);
		}
#endif
		return;
	}
	/* initial 'palette' setting */
	if (makemono) {
		if (!(palette = malloc(256))) {
			I_Error("mapping alloc failed\n");
		}
		/* color->mono mapping with gamma and hue weighting */
		for (i = 0; i < 256; i++) {
			palette[i] = i;
			value =  gamma[*pal++] * 307UL;
			value += gamma[*pal++] * 599UL;
			value += gamma[*pal++] * 118UL;
			grayscale[i] = value >> 10;
		}
	} else {
		/* generate palette and mapping */
		for (i = 0; i < 256; i++) {
			rgb[i].red   = gamma[*pal++];
			rgb[i].green = gamma[*pal++];
			rgb[i].blue  = gamma[*pal++];
		}
		palette = w_allocMap(wvis, 256, rgb, NULL);
		if (!palette) {
			I_Error("color map setting failed\n");
		}
	}
	/* silly place for window open, but... */
	w_open(wvis, UNDEF, UNDEF);
}

void I_CheckRes()
{
}

void InitGraphLib(void)
{
}

void I_InitGraphics(void)
{
	WSERVER *server;
	int attr;

	fprintf(stderr, "I_InitGraphics: Init W-visual.\n");

	if (M_CheckParm("-debug") > 0) {
		w_trace(1);
	}
	realwidth = screenwidth;
	realheight = screenheight;
	scale = M_CheckParm("-scale") > 0;
	if (scale) {
		realwidth *= 2;
		realheight *= 2;
	}

	if (!(server = w_init())) {
		I_Error("Unable to connect W!\n");
	}
	if (server->type == BM_PACKEDMONO) {
		makemono = 1;
		w_ditherOptions(grayscale, 0);
	}

	attr = W_MOVE | W_TITLE | W_CLOSE | EV_KEYS | EV_MOUSE | EV_MODIFIERS;
	if (!M_CheckParm("-nomouse")) {
		 attr |= EV_MMOVE;
	}
	if (!(server->flags & WSERVER_KEY_MAPPING)) {
		fprintf(stderr, "  W server doesn't support key release events,\n");
		fprintf(stderr, "  forcing mouse input on.\n");
		attr |= EV_MMOVE;
	}
	if (!(wvis = w_create(realwidth, realheight, attr))) {
		I_Error("Unable to create a window!\n");
	}
#ifdef SHOW_FPS
	if ((font = w_loadfont(NULL, 0, 0))) {
		w_setfont(wvis, font);
		w_settextstyle(wvis, F_REVERSE);
	} else {
		I_Error("Unable to load default font!\n");
	}
#endif

	if (!(frame = w_allocbm(realwidth, realheight, BM_DIRECT8, 256))) {
		I_Error("Unable to allocate conversion bitmap!\n");
	}
	if (!(screen = malloc(screenwidth*screenheight))) {
		I_Error("Unable to allocate screen memory?!\n");
	}
}
