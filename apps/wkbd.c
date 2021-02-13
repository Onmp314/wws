#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <Wlib.h>

#ifndef DATADIR
# error "ERROR: DATADIR undefined"
#endif
#define KBD_DATADIR DATADIR "/wkbd"

struct keycolumn {
        int xoffset;
        int scancode;
};


struct keyrow {
        int yoffset;
        int height;
        struct keycolumn columns[12];
};

#define UNK 1000
#define CAPS 1001
#define SHIFT 1002
/* #define INTL 1003 */
#define INTL '&'
#define NUM 1004

#define SCANCODES 41

static const struct keyrow keyrows[4] = {
        {0, 15, 
         {{0, 0}, {14, 1}, {28, 2}, {42, 3}, {56, 4}, {70, 5}, {84, 6}, {98, 7}, {112, 8}, {126, 9}, {140, 10}, {999, -1}}},
        {15, 15, 
         {{0, 11}, {14, 12}, {28, 13}, {42, 14}, {56, 15}, {70, 16}, {84, 17}, {98, 18}, {112, 19}, {126, 20}, {140, 21}, {999, -1}}},
        {30, 15, 
         {{0, 22}, {19, 23}, {33, 24}, {47, 25}, {61, 26}, {75, 27}, {89, 28}, {103, 29}, {117, 30}, {131, 31}, {145, 32}, {999, -1}} },
        {45, 15, 
         {{0, 33}, {21, 34}, {36, 35}, {85, 36}, {103, 37}, {117, 38}, {131, 39}, {145, 40}, {999, -1}}}
};

static const int unshifted[SCANCODES] = 
{'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '\177', 
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '-', '\r',
 CAPS, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', ';',
 SHIFT, INTL, ' ', NUM, '\'', '=', '\\', '/'};

static const int shifted[SCANCODES] =
{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '\177', 
 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '_', '\r',
 CAPS, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', ':',
 SHIFT, INTL, ' ', NUM, '"', '+', '|', '?'};

static const int numed[SCANCODES] =                             
{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\177', 
 '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '\r',
 /* SYM, 'div', 'plusminus', 'sqrt', 'perthou', 'yen', 'pound', '[', ']', '{', '}', */
 CAPS, UNK, UNK, UNK, UNK, UNK, UNK, '[', ']', '{', '}',
 SHIFT, INTL, ' ', NUM, '<', '>', '`', '~'};

struct layout_state {
        const char *name;
        const char *filename;
        const int *scancode_translations;
        int ctrl;
        int caps_layout, shift_layout, num_layout;
};

static const struct layout_state layout_states[] = {
        {
                "unshifted",
                "key0.pbm",
                unshifted,
                0,
                2, 1, 3
        }, 
        {
                "shifted",
                "key1.pbm",
                shifted,
                0,
                2, 0, 3
        }, 
        {
                "ctrl",
                "key2.pbm",
                unshifted,
                1, /* ctrl mapping */
                0, 1, 3
        },
        {
                "num",
                "key4.pbm",
                numed,
                0,
                2, 1, 0
        }
};

int current_layout = 0;
        
int kbdfd;

static int open_console(char *consolename, const int mode)
{
        if ((kbdfd=open(consolename, mode)) < 0) {
                printf("couldn't open console %s\n", consolename);
                exit(1);
                return -1;
        }
  return kbdfd;
}

#ifdef BROKEN
static void push_character(char c)
{
        char buf[2];
        int res;

        buf[0] = 13; /* push char */
        buf[1] = c;
        if ((res = ioctl(kbdfd, TIOCLINUX, buf)) < 0) {
                printf("got bad result %d\n", res);
                exit(1);
        }
}
#else
static void push_character(char c)
{
        int res;

        if ((res = ioctl(kbdfd, TIOCSTI, &c)) < 0) {
                printf("got bad result %d\n", res);
                exit(1);
        }
}
#endif

WSERVER *server;                
WWIN *win;

static void
display_layout(int layout) 
{
        char buf[128];
        BITMAP *bm;
        
        sprintf(buf, "%s/%s", KBD_DATADIR, layout_states[layout].filename);

        bm = w_readpbm(buf);
        if (!bm) {
                fprintf(stderr, "lost reading bm %s\n", buf);
                exit(1);
        }

        if (w_putblock(bm, win, 0, 0)) {
                fprintf(stderr, "\n%s: image putblock failed\n", buf);
                exit(1);
        }

        w_freebm(bm);

        w_flush();
}


static void
process_scancode(int scancode)
{
        int c;

        c = layout_states[current_layout].scancode_translations[scancode];

        switch (c) {
        case UNK:
                printf("key with unknown translation pressed\n");
                return;
        case CAPS:
                current_layout = layout_states[current_layout].caps_layout;
                display_layout(current_layout);
                return;
        case SHIFT:
                current_layout = layout_states[current_layout].shift_layout;
                display_layout(current_layout);
                return;
        case NUM:
                current_layout = layout_states[current_layout].num_layout;
                display_layout(current_layout);
                return;
        }

        if (layout_states[current_layout].ctrl) {
                c &= ' ' - 1;
                current_layout = 0;
                display_layout(current_layout);
        }

        /* printf("pushing character %c (%d)\n", c, c); */
        push_character(c);
}


static void 
mouse_hit(int x, int y) {
        int row, column;

        for (row = 0; row < 4; row++) {
                if (y >= keyrows[row].yoffset && y < keyrows[row].yoffset+keyrows[row].height) {
                        for (column = 0; column < 12; column++) {
                                if (keyrows[row].columns[column].xoffset == 999) {
                                        printf("off end of row\n");
                                        return;
                                }
                                if (x < keyrows[row].columns[column + 1].xoffset) {
                                        int scancode = keyrows[row].columns[column].scancode;
                                        /* printf("got scancode %d\n", scancode); */
                                        process_scancode(scancode);
                                        return;
                                }
                        }
                }
        }

        printf("off bottom\n");
}
                                

#define BM_WIDTH 160
#define BM_HEIGHT 61

int
main(int argc, char *argv[])
{
        if (argc != 2) {
                fprintf(stderr, "usage: wkbd /dev/tty1\n");
                exit(1);
        }

        open_console(argv[1], O_WRONLY);

        if (!(server = w_init())) {
                fprintf(stderr, "\n%s: W server connection failed\n", *argv);
                return -1;
        }

        win = w_create(BM_WIDTH, BM_HEIGHT, EV_MOUSE | W_MOVE | W_CLOSE);
        if (!win) {
                fprintf(stderr, "\n%s: unable to open (%d,%d) window\n",
                        *argv, BM_WIDTH, BM_HEIGHT);
                return -1;
        }

        if (w_open(win, -4, 95) < 0) {
                w_delete(win);
                return -1;
        }

        current_layout = 0;
        display_layout(0);

        while (1) {
                WEVENT *ev;
                if ((ev = w_queryevent(NULL, NULL, NULL, -1))) {
                  /* printf("got SOMETHING\n"); */
                        if (ev->type == EVENT_MPRESS) {
                                short x, y;
                                x = ev->x;
                                y = ev->y;
                                printf("at %d %d\n", x, y);
                                mouse_hit(x, y);
                        }
                }
        }

}
