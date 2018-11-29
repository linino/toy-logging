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
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tracing.h"
#include "sys.h"

#define SHARED_MEM_SIZE 4096

struct test1_args {
	const char *str;
	int num;
};

/* Callback for test1 tracepoint. MUST return number of consumed bytes */
int test1_cb(const struct traced_event *te, const void *_args)
{
	struct test1_args args;

	memcpy(&args, _args, sizeof(args));

	printf("EVENT %s -> %s, %d\n", te->name, args.str, args.num);
	return sizeof(args);
}

int test1_copy_args(struct test1_args *_a, const char *str, int num)
{
	struct test1_args a;

	a.str = str;
	a.num = num;
	memcpy(_a, &a, sizeof(a));
	return sizeof(a);
}
declare_trace_evt(test1, PROTO(const char *str, int num), str, num);


struct test2_args {
	const char *str;
};

/* Callback for test1 tracepoint. MUST return number of consumed bytes */
int test2_cb(const struct traced_event *te, const void *_args)
{
	struct test2_args args;

	memcpy(&args, _args, sizeof(args));

	printf("EVENT %s -> %s\n", te->name, args.str);
	return sizeof(args);
}

int test2_copy_args(struct test2_args *_a, const char *str)
{
	struct test2_args a;

	a.str = str;
	memcpy(_a, &a, sizeof(a));
	return sizeof(a);
}
declare_trace_evt(test2, PROTO(const char *str), str);


static void toy_loop(const char *name)
{
	int counter;
	
	for (counter = 0; ; counter++) {
		sys_sleep(1);
		if (!(counter & 1))
			test1_trace(name, counter);
		else
			test2_trace(name);
	}
}

int main(int argc, char *argv[])
{
	int stat;
	void *shared_mem;

	
	if (sys_init() < 0)
		return 127;

	shared_mem = sys_setup_tracing_memory(SHARED_MEM_SIZE);

	if (!shared_mem)
		return 127;

	if (tracing_init(shared_mem, SHARED_MEM_SIZE) < 0) {
		printf("ERROR in tracing init\n");
		return -1;
	}

	/* Fork a couple of infinite loops */
	stat = sys_fork_loop(toy_loop, "toy1");
	if (stat < 0) {
		printf("ERROR in sys_fork_loop()\n");
		return stat;
	}

	stat = sys_fork_loop(toy_loop, "toy2");
	if (stat < 0) {
		printf("ERROR in sys_fork_loop()\n");
		return stat;
	}

	/* And start tracing */
	while(1)
		tracing_loop();
}
