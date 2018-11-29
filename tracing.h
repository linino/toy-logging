/*
 * "Toy" tracing sample program.
 *
 * Copyright 2018 dog hunter LLC and the Linino community
 *
 * Linino.org is a dog hunter sponsored community project
 *
 * Author: Davide Ciminaghi <davide@linino.org>
 *
 * SPDX-License-Identifier: GPL-2.0
 */
#ifndef __TRACING_H__
#define __TRACING_H__

#include <stdint.h>

#define cat(a,b) a ## b
#define xcat(a,b) cat(a,b)
#define BIT(n) (1 << (n))

/*
 * Max 255 traced events, so that we can select an event with an 8 bits number
 * and save RAM in tracing buffer.
 * One event (the dummy one) is reserved for internal usage in tracing.c
 */
#define MAX_EVENTS 255
typedef uint8_t tracepoint_descriptor_offset_t;

#define event_name(n) xcat(n,_te)
#define event_trace_fun(n) xcat(n,_trace)
#define event_flags(n) xcat(event_name(n), _flags)
#define event_ptr(n) xcat(n,_te_ptr)
#define event_args_struct(n) struct xcat(n,_args)
#define event_copy_args_fun(n) xcat(n,_copy_args)
#define event_cb_fun(n) xcat(n,_cb)
#define PROTO(args...) (args)

#define EVENT_ENABLED BIT(0)
/* Callback invoked immediately when tracepoint hit */
#define EVENT_IMMEDIATE BIT(1)

struct traced_event {
	const char *name;
	unsigned char *flags;
	int args_size;
	int (*cb)(const struct traced_event *, const void *args);
};

/* Defined by the linker */
extern const struct traced_event \
traced_events_start[], traced_events_end[];

/*
 * Get some space in tracing circular buffer, receives event offset and
 * event data size. Returns pointer to event payload area.
 * Locks events buffer.
 */
extern void *tracing_get_event(tracepoint_descriptor_offset_t evt_id,
			       unsigned int event_data_sz);
/*
 * Receives pointer returned by tracing_get_event(). Commits written event
 * to events buffer and unlocks such buffer.
 */
extern void tracing_event_written(void *, unsigned int event_data_sz);


static inline void event_enable(const struct traced_event *te)
{
	*te->flags |= EVENT_ENABLED;
}

static inline void event_disable(const struct traced_event *te)
{
	*te->flags &= ~EVENT_ENABLED;
}

static inline int is_event_enabled(const struct traced_event *te)
{
	return *te->flags & EVENT_ENABLED;
}

/*
 * n -> event name
 * s -> event args structure
 * args -> variable list of event arguments
 */
#define declare_trace_fun(n,tproto,args...)				\
	static inline int event_trace_fun(n) tproto			\
	{								\
		const struct traced_event *te = &event_name(n);		\
		tracepoint_descriptor_offset_t o =			\
			te - traced_events_start;			\
		event_args_struct(n) *a;				\
		int ret = -1;						\
									\
		if (!is_event_enabled(te))				\
			/* Disabled event, ignore it */			\
			return 0;					\
		a = tracing_get_event(o, sizeof(*a));			\
		if (!a) {						\
			printf("WARNING: %s, NO SPACE FOR TRACE\n", te->name); \
			return ret;					\
		}							\
		ret = event_copy_args_fun(n)(a, args);			\
		tracing_event_written(a, sizeof(*a));			\
		return ret;						\
	}

#define declare_trace_evt(n,proto,args...)				\
	unsigned char event_flags(n);					\
	const struct traced_event event_name(n)				\
	__attribute__((section(".traced_events"))) = {			\
		.name = #n,						\
		.flags = &event_flags(n),				\
		.cb = event_cb_fun(n),					\
	};								\
	declare_trace_fun(n, proto,args);

/* This must be called from an independent process' context */
extern int tracing_loop(void);

/* Guess what */
extern int tracing_init(void *area, unsigned int size);

#endif /* __TRACING_H__ */
