/*
 *  Test protocol
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#ifndef TEST_TESTPROTOCOL_H_
#define TEST_TESTPROTOCOL_H_

#include <xenctrl.h>
#include <xen/io/ring.h>
#include <xen/grant_table.h>

/// @cond HIDDEN_SYMBOLS

#define XENTEST_CMD1	0x10
#define XENTEST_CMD2	0x11
#define XENTEST_CMD3	0x12

struct xentest_command1_req
{
	uint32_t u32data1;
	uint32_t u32data2;
};

struct xentest_command2_req
{
	uint64_t u64data1;
};

struct xentest_command3_req
{
	uint16_t u16data1;
	uint16_t u16data2;
	uint32_t u32data3;
};

struct xentest_req
{
	uint32_t id;
	uint32_t seq;
	union
	{
		struct xentest_command1_req command1;
		struct xentest_command2_req command2;
		struct xentest_command3_req command3;
		uint8_t reserved1[56];
	} op;
};

struct xentest_rsp
{
	uint32_t status;
	uint32_t u32data;
	uint32_t seq;
	uint8_t reserved1[52];
};

DEFINE_RING_TYPES(xen_test, struct xentest_req, struct xentest_rsp);

#define XENTEST_EVT1	0x10
#define XENTEST_EVT2	0x11
#define XENTEST_EVT3	0x12

struct xentest_event1
{
	uint32_t u32data1;
	uint32_t u32data2;
};

struct xentest_event2
{
	uint64_t u64data1;
};

struct xentest_event3
{
	uint16_t u16data1;
	uint16_t u16data2;
	uint32_t u32data3;
};

struct xentest_evt
{
	uint32_t id;
	uint32_t seq;
	union
	{
		struct xentest_event1 event1;
		struct xentest_event2 event2;
		struct xentest_event3 event3;
		uint8_t reserved1[56];
	} op;
};

struct xentest_event_page
{
	uint32_t in_cons;
	uint32_t in_prod;
	uint8_t reserved[56];
};

#define XENTEST_EVENT_PAGE_SIZE 4096
#define XENTEST_IN_RING_OFFS (sizeof(struct xentest_event_page))
#define XENTEST_IN_RING_SIZE (XENTEST_EVENT_PAGE_SIZE - XENTEST_IN_RING_OFFS)
#define XENTEST_IN_RING_LEN (XENTEST_IN_RING_SIZE / sizeof(struct xentest_evt))
#define XENTEST_IN_RING(page) \
	((struct xentest_evt *)((char *)(page) + XENTEST_IN_RING_OFFS))
#define XENTEST_IN_RING_REF(page, idx) \
	(XENTEST_IN_RING((page))[(idx) % XENTEST_IN_RING_LEN])

/// @endcond

#endif /* TEST_TESTPROTOCOL_H_ */
