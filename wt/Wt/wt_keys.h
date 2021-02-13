/* 
 * Here are standardized some shortcut and additional special keys for text
 * input in case W server doesn't support special key mapping (ie just
 * transmits keys from console below).  All codes use only one byte and
 * values are all below 32 (ie.  so called `control' keys).  Most of them
 * are similar to default Emacs keyboard bindings.
 */

/* cursor movement */
#define KEY_LEFT	('B' & 0x1f)	/* _b_ackward char */
#define KEY_RIGHT	('F' & 0x1f)	/* _f_orward char */
#define KEY_HOME	('A' & 0x1f)
#define KEY_END		('E' & 0x1f)	/* _e_nd of line */
#define KEY_UP		('P' & 0x1f)	/* _p_revious line */
#define KEY_DOWN	('N' & 0x1f)	/* _n_ext line */

/* these are a bit strange because I wanted ones that won't clash
 * with other shortcuts and on Emacs PGUP is two characters anyway...
 */
#define KEY_PGUP	('<' & 0x1f)	/* in emacs: ESC-V */
#define KEY_PGDOWN	('>' & 0x1f)	/* in emacs: ^V */

/* then some editing keys */
#define KEY_BS		'\b'		/* in emacs: ^H */
#define KEY_DEL		'\177'		/* in emacs: ^D */
#define KEY_TRANSPOSE	('T' & 0x1f)	/* transpose letterpair on cursor */
#define KEY_KILLINE	('K' & 0x1f)	/* kill (rest of) line */


/* If application is supposed to handle keys you won't, ESC is a good
 * prefix, although it clashes a bit with VTxxx special keys.
 *
 * Later on W server will map them, so that shouldn't be as much of a
 * problem (multi-character keys are anyway a pain in the ***, better
 * have character defined so that all keys fit into one).
 *
 * NOTE: this isn't included into DESTRUCTIVE macro.
 */
#define KEY_CLEAR	'\33'		/* ESC clears field */


/* DOS/Atari editor style bindings. */
#define KEY_CUT		('X' & 0x1f)	/* cut to clipboard */
#define KEY_COPY	('C' & 0x1f)	/* copy to clipboard */
#define KEY_PASTE	('V' & 0x1f)	/* paste from clipboard */
#define KEY_UNDO	('U' & 0x1f)	/* line editing undo */
#define KEY_FORMAT	('Z' & 0x1f)	/* format paragraph */
