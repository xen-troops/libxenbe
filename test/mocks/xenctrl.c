/*
 * xenctrl.c
 *
 *  Created on: Feb 27, 2017
 *      Author: al1
 */

#include <stdlib.h>

#include <xenctrl.h>

struct xc_interface_core
{
	int flags;
	xentoollog_logger *error_handler,   *error_handler_tofree;
	xentoollog_logger *dombuild_logger, *dombuild_logger_tofree;
	struct xc_error last_error; /* for xc_get_last_error */
	FILE *dombuild_logger_file;
	const char *currently_progress_reporting;
};

int xc_interface_close(xc_interface *xch)
{
	return 0;
}

int xc_domain_getinfolist(xc_interface *xch,
						  uint32_t first_domain,
						  unsigned int max_domains,
						  xc_domaininfo_t *info)
{
	return 1;
}

xc_interface *xc_interface_open(xentoollog_logger *logger,
								xentoollog_logger *dombuild_logger,
								unsigned open_flags)
{
	return malloc(sizeof(xc_interface));
}
