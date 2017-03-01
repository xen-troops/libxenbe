/*
 * xenevtchn.c
 *
 *  Created on: Feb 27, 2017
 *      Author: al1
 */

#include <stdlib.h>

#include <xenevtchn.h>

struct xenevtchn_handle
{
	int fd;
};

xenevtchn_port_or_error_t
xenevtchn_bind_interdomain(xenevtchn_handle *xce, uint32_t domid,
						   evtchn_port_t remote_port)
{
	return 5;
}

int xenevtchn_unbind(xenevtchn_handle *xce, evtchn_port_t port)
{
	return 0;
}

int xenevtchn_notify(xenevtchn_handle *xce, evtchn_port_t port)
{
	return 0;
}

int xenevtchn_fd(xenevtchn_handle *xce)
{
	return 7;
}

xenevtchn_handle *xenevtchn_open(struct xentoollog_logger *logger,
								 unsigned open_flags)
{
	return malloc(sizeof(xenevtchn_handle));
}

int xenevtchn_unmask(xenevtchn_handle *xce, evtchn_port_t port)
{
	return 0;
}

int xenevtchn_close(xenevtchn_handle *xce)
{
	return 0;
}

xenevtchn_port_or_error_t xenevtchn_pending(xenevtchn_handle *xce)
{
	return 8;
}
