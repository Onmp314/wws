#include "wt_keys.h"			/* cursor movement keys */

/* which destructive keys from wt_keys.h I use... */
#define DESTRUCTIVE(k) (\
	k == WKEY_DEL  || k == KEY_DEL || k == KEY_BS ||\
	k == KEY_CLEAR || k == KEY_KILLINE)

#define MOVEMENT(k) (\
	k == WKEY_LEFT || k == KEY_LEFT || k == WKEY_RIGHT || k == KEY_RIGHT ||\
	k == WKEY_HOME || k == KEY_HOME || k == WKEY_END   || k == KEY_END   ||\
	k == WKEY_UP   || k == KEY_UP   || k == WKEY_DOWN  || k == KEY_DOWN  ||\
	k == WKEY_PGUP || k == KEY_PGUP || k == WKEY_PGDOWN || k == KEY_PGDOWN)

