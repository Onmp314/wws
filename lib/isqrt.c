/*
 * isqrt.c, a part of the W Window System
 *
 * -- a quick integer square root using binomial theorem:  O( 1/2 log2 N )
 * by Peter Heinrich in Dr. Dobbs Journal 1996
 */

#include "Wlib.h"

ulong isqrt(ulong N)
{
	ulong l2, u, v, u2, n;
	if(2 > N) {
		return N;
	}
	u = N;
	l2 = 0;
	/* 1/2 * log_2 N = highest bit in the result */
	while((u >>= 2)) {
		l2++;
	}
	u = 1L << l2;
	v = u;
	u2 = u << l2;
	while(l2--) {
		v >>= 1;
		n = (u + u + v) << l2;
		n += u2;
		if(n <= N) {
			u += v;
			u2 = n;
		}
	}
	return u;
}
