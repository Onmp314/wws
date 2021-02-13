/*
 * hash.c, a part of the W Window System
 *
 * Copyright (C) 1995 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- generic type "hash table"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <sys/types.h>
#include "hash.h"


/*
 * create a new hashtable with size `size', which should be a prime number
 */
hashtab_t *
hashtab_create (long size)
{
	hashtab_t *htab;

	if (size <= 0)
		return NULL;
	htab = malloc (sizeof (hashtab_t) + size * sizeof (hashnode_t *));
	if (!htab)
		return NULL;
	htab->size = size;
	memset (htab->bucks, 0, sizeof (hashnode_t *) * size);
	return htab;
}


/*
 * delete a hash table. If `f' is nonzero, then it will be called
 * for every node in the hash table.
 */

void
hashtab_delete (hashtab_t *htab, void (*f) (void *))
{
	hashnode_t *this;
	int i;

	for (i = 0; i < htab->size; ++i) {
		while ((this = htab->bucks[i])) {
			if (f) (*f) (this->val);
			htab->bucks[i] = this->next;
			free (this);
		}
	}
	free (htab);
}


/*
 * Insert a (key, val) pair into a hash table.
 */

int
hash_insert (hashtab_t *htab, u_short key, void *val)
{
	hashnode_t *hn;
	int i;

	hn = malloc (sizeof (hashnode_t));
	if (!hn)
		return -1;
	hn->key = key;
	hn->val = val;

	i = key % htab->size;
	hn->next = htab->bucks[i];
	htab->bucks[i] = hn;
	return 0;
}

/*
 * delete a (key, *) pair from the hash table. Returns the `val' field
 * of the hash node if a matching one is found, NULL otherwise.
 */
void *
hash_delete (hashtab_t *htab, u_short key)
{
	hashnode_t **prev, *this;
	void *val;

	prev = &htab->bucks[key % htab->size];
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this->key == key) {
			*prev = this->next;
			val = this->val;
			free (this);
			return val;
		}
	}
	return NULL;
}

/*
 * look for a (key, *) pair in the hash table. Returns the `val' field
 * of the hash node if a matching one is found, NULL otherwise.
 */
void *
hash_lookup (hashtab_t *htab, u_short key)
{
	hashnode_t *this;

	for (this = htab->bucks[key % htab->size]; this; this = this->next) {
		if (this->key == key)
			return this->val;
	}
	return NULL;
}
