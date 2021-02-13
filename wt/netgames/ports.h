/*
 * Networked, turn based, two player games.
 *
 * Here are IDs for the known games so that new interfaces / servers for
 * that game (with strictly defined rules so that both ends have always the
 * same board state) can just include this file.
 *
 * (w) 1996 by Eero Tamminen
 */

#define GOBANG_ID	0x474f4e47			/* "GONG" (GObaNG) */
#define GOMOKU_ID	0x474f4b55			/* "GOKU" (GOmoKU) */
#define PENTE_ID	0x50455445			/* "PETE" (PEnTE) */
#define CHESS_ID	0x43485353			/* "CHSS" (CHeSS) */
/* "GO" + board size used in ASCII (default = "19") */
#define GO_ID		0x474f3139
