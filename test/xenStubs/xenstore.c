/*
 * xenstore.c
 *
 *  Created on: Feb 27, 2017
 *      Author: al1
 */

#include <stdlib.h>

#include <xenstore.h>

struct xs_handle
{
	int fd;
};

struct xs_handle *xs_open(unsigned long flags)
{
	return malloc(sizeof(struct xs_handle));
}

void xs_close(struct xs_handle *xsh)
{
	free(xsh);
}

bool xs_rm(struct xs_handle *h, xs_transaction_t t, const char *path)
{
	return true;
}

