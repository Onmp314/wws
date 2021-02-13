/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* icon.c - Deals with our icon.  Setup and execution.
******************************************************************************/

#include <stdlib.h>
#include <Wlib.h>
#include "main.h"
#include "proto.h"

#include "bitmaps/dragon_icon32.c"

static BITMAP dragon_icon32_bitmap = {
  32, 32,
  BM_PACKEDMONO, 4, 1, 1,
  dragon_icon32_bits
};


void Icon_Setup(void)
/******************************************************************************
* Called to handle all of the Icon setup.
******************************************************************************/
{
    DEBUG_CALL(Icon_Setup);

    if ((Icon = w_create(32, 32, (W_MOVE|EV_MOUSE))) == 0) {
	fprintf(stderr, "Can't create icon.\n");
	exit(1);
    }

    w_putblock(&dragon_icon32_bitmap, Icon, 0, 0);

    if (Dragon_Resources.Iconic) {
      /* Immediately show the icon. */
      short x, y, w, h;
      scan_geometry(Dragon_Resources.Geometry, &w, &h, &x, &y);
      w_open(Icon, x, y);
    }

    DEBUG_RETURN(Icon_Setup);

} /* Icon_Setup */

void Iconify(void)
/******************************************************************************
* Called to iconify/deiconify.
******************************************************************************/
{
    short x, y;

    DEBUG_CALL(Iconify);

    if (Dragon_Resources.Iconic) {
	w_querywindowpos(Icon, 1, &x, &y);
	w_close(Icon);
	w_open(Board, x, y);
    } else {
	w_querywindowpos(Board, 1, &x, &y);
	w_close(Board);
	w_open(Icon, x, y);
    }

    Dragon_Resources.Iconic = !Dragon_Resources.Iconic;

    DEBUG_RETURN(Iconify);

} /* Icon_Setup */

