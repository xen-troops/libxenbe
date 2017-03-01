/*
 * xengnttab.c
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#include <stdlib.h>

#include <xengnttab.h>

struct xengntdev_handle {
	int fd;
};

void *xengnttab_map_domain_grant_refs(xengnttab_handle *xgt,
									  uint32_t count,
									  uint32_t domid,
									  uint32_t *refs,
									  int prot)
{
	return 0;
}

int xengnttab_unmap(xengnttab_handle *xgt, void *start_address, uint32_t count)
{
	return 0;
}

int xengnttab_close(xengnttab_handle *xgt)
{
	return 0;
}

xengnttab_handle *xengnttab_open(struct xentoollog_logger *logger,
								 unsigned open_flags)
{
	return malloc(sizeof(xengnttab_handle));
}
