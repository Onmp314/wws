/*
 * hash.h, a part of the W Window System
 *
 * Copyright (C) 1995 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 */
#ifndef _HASH_H
#define _HASH_H

typedef struct _hashnode_t {
	u_short key;
	void *val;
	struct _hashnode_t *next;
} hashnode_t;

typedef struct _hashtab_t {
	long size;
	hashnode_t *bucks[0];
} hashtab_t;

/*
 * `size' should be a prime number. The bigger it is the
 * faster the lookup and the more memory is used. `size' should
 * be greater than the average number of entries you expect to
 * be stored in the table at the same time.
 */
extern hashtab_t *hashtab_create (long size);
extern void       hashtab_delete (hashtab_t *htab, void (*f) (void *));

extern int   hash_insert (hashtab_t *htab, u_short key, void *val);
extern void *hash_delete (hashtab_t *htab, u_short key);
extern void *hash_lookup (hashtab_t *htab, u_short key);

#endif
