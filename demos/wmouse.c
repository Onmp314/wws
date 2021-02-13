/*
 * Mouse pointer demo
 *
 * (w) 1998 by Jan Paul Schmidt
 */

#include <stdlib.h>
#include <Wlib.h>


#define WORD(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
  (a<<15|b<<14|c<<13|d<<12|e<<11|f<<10|g<<9|h<<8|i<<7|j<<6|k<<5|l<<4|m<<3|n<<2|o<<1|p)

WMOUSE my_mouse = {
  -8,
  -8,
  {
    WORD (0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0),
    WORD (0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0),
    WORD (0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0),
    WORD (0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
    WORD (0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0),
    WORD (0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0),
    WORD (0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0),
    WORD (0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0),
  },
  {
    WORD (0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
    WORD (0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0),
    WORD (0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0),
    WORD (0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0),
    WORD (0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0),
    WORD (0,1,0,0,1,1,0,0,0,0,1,1,0,0,1,0),
    WORD (1,0,0,0,1,1,0,0,0,0,1,1,0,0,0,1),
    WORD (1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1),
    WORD (1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1),
    WORD (1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1),
    WORD (0,1,0,0,1,0,0,0,0,0,0,1,0,0,1,0),
    WORD (0,1,0,0,0,1,0,0,0,0,1,0,0,0,1,0),
    WORD (0,0,1,0,0,0,1,1,1,1,0,0,0,1,0,0),
    WORD (0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0),
    WORD (0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0),
    WORD (0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0),
  }
};



int main (int argc, char *argv []) {
  WWIN *window;
  WWIN *child [4];
  short i;

  w_init ();
  window = w_create (100, 100, W_TITLE | W_MOVE | W_CLOSE | EV_KEYS);

  for (i = 0; i < 4; i++) {
    child [i] = w_createChild (window, 50, 50, W_NOBORDER | EV_KEYS);
    w_open (child [i], (i >> 1) * 50, (i & 1) * 50);
  }

  w_setmousepointer (child [0], MOUSE_ARROW, NULL);
  w_setmousepointer (child [1], MOUSE_BUSY, NULL);
  w_setmousepointer (child [2], MOUSE_MOVE, NULL);
  w_setmousepointer (child [3], MOUSE_USER, &my_mouse);
  w_open (window, UNDEF, UNDEF);
  w_queryevent (NULL, NULL, NULL, -1);
  w_delete (window);

  exit (EXIT_SUCCESS);
}
