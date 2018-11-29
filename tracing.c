/*
 * "Toy" tracing system.
 *
 * Copyright 2018 dog hunter LLC and the Linino community
 *
 * Linino.org is a dog hunter sponsored community project
 *
 * Author: Davide Ciminaghi <davide@linino.org>
 *
 * SPDX-License-Identifier: GPL-2.0
 */
#include <string.h>
#include <stdio.h>

#include "tracing.h"
#include "circ_buf.h"
#include "sys.h"

#define DUMMY_EVENT 0xff

struct tracing_data {
	struct circ_buf cb;
	/* Must be a power of 2 !! */
	char buf[2048];
};

static struct tracing_data *_td;

#define BUFSIZE sizeof(_td->buf)

static inline int space(struct tracing_data *td)
{
	return CIRC_SPACE(td->cb.head, td->cb.tail, BUFSIZE);
}

static inline int space_to_end(struct tracing_data *td)
{
	return CIRC_SPACE_TO_END(td->cb.head, td->cb.tail, BUFSIZE);
}

int tracing_init(void *shared_mem, unsigned int size)
{
	if (sizeof(tracepoint_descriptor_offset_t) != 1) {
		printf("CURRENT IMPLEMENTATION ASSUMES EVENT ID IS 1 BYTE\n");
		return -1;
	}
	if (size < sizeof(struct tracing_data)) {
		printf("%s: NOT ENOUGH SPACE\n", __func__);
		return -1;
	}
	_td = shared_mem;
	memset(_td, 0, sizeof(*_td));
	return 0;
}

/* Assumes lock taken */
static void pad_with_dummy(struct tracing_data *td)
{
	int len = space_to_end(_td);
	
	memset(&td->buf[td->cb.head], DUMMY_EVENT, len);
	td->cb.head = (td->cb.head + len) & (BUFSIZE - 1);
}

void *tracing_get_event(tracepoint_descriptor_offset_t evt_id,
			unsigned int event_data_sz)
{
	int required = sizeof(evt_id) + event_data_sz;
	
	sys_lock_tracing_buf();
	do {
		if (space(_td) < required) {
			sys_unlock_tracing_buf();
			return NULL;
		}
		if (space_to_end(_td) >= required)
			break;
		/*
		 * Insert dummy events up to the end of the buffer
		 * We need contiguous space for writing
		 */
		pad_with_dummy(_td);
	} while(1);

	/* WE DON'T UPDATE HEAD HERE !! BUFFER IS LOCKED ON RETURN !! */
	_td->buf[_td->cb.head] = evt_id;
	return &_td->buf[_td->cb.head + 1];
}

void tracing_event_written(void *ptr, unsigned int event_data_sz)
{
	_td->cb.head = (_td->cb.head + sizeof(tracepoint_descriptor_offset_t) +
			event_data_sz) & (BUFSIZE - 1);
	sys_unlock_tracing_buf();
}


int tracing_loop(void)
{
	tracepoint_descriptor_offset_t evt_id;
	const struct traced_event *te;
	int sz;

	while(1) {
		if (!CIRC_CNT(_td->cb.head, _td->cb.tail, BUFSIZE))
			continue;
		/* Events available */
		/* Skip dummy events */
		do {
			evt_id = _td->buf[_td->cb.tail];
			_td->cb.tail = (_td->cb.tail + 1) & (BUFSIZE - 1);
		} while(evt_id == DUMMY_EVENT);
		te = &traced_events_start[evt_id];
		sz = te->cb(te, &_td->buf[_td->cb.tail]);
		_td->cb.tail = (_td->cb.tail + sz) & (BUFSIZE - 1);
	}
}
