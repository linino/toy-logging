/*
 * "Toy" tracing sample program, unix related functions
 *
 * Copyright 2018 dog hunter LLC and the Linino community
 *
 * Linino.org is a dog hunter sponsored community project
 *
 * Author: Davide Ciminaghi <davide@linino.org>
 *
 * SPDX-License-Identifier: GPL-2.0
 */
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "tracing.h"

struct shared_data {
	sem_t bufsem;
};

struct shared_data *local_shared_area;

void *sys_setup_tracing_memory(unsigned int size)
{
	int fd, stat;
	void *shm_area;
	/* Get extra space for sys stuff */
	int actual_size = size + 4096;

	/* Setup the common memory area */
	fd = open("/tmp/toy-logging", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "opening tmpfs buffer file: %s\n",
			strerror(errno));
		exit(5);
	}
	/* Resize shared memory area */
	stat = fallocate(fd, 0, 0, actual_size);
	if (stat < 0) {
		fprintf(stderr, "resizing buffer: %s\n", strerror(errno));
		exit(6);
	}
	shm_area = mmap(NULL, actual_size, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (shm_area == MAP_FAILED) {
		fprintf(stderr, "mapping tmpfs buffer file: %s\n",
			strerror(errno));
		exit(7);
	}
	close(fd);
	memset(shm_area, 0, sizeof(*shm_area));
	/* sys area is just after the common shared memory */
	local_shared_area = (struct shared_data *)&((char *)shm_area)[size];
	if (sem_init(&local_shared_area->bufsem, 1, 1) < 0) {
		fprintf(stderr, "error inizializing buffer semaphore\n");
		exit(8);
	}
	return shm_area;
}

/* Declare a unix specific event, which traces sigusr1 */
struct sig_args {
	struct timeval tv;
};

/* Callback for sig tracepoint. MUST return number of consumed bytes */
int sig_cb(const struct traced_event *te, const void *_args)
{
	struct sig_args args;

	memcpy(&args, _args, sizeof(args));

	printf("EVENT %s -> TS = %ld.%06ld\n", te->name, args.tv.tv_sec,
	       args.tv.tv_usec);
	return sizeof(args);
}

int sig_copy_args(struct sig_args *_a, struct timeval tv)
{
	struct sig_args a;

	a.tv = tv;
	memcpy(_a, &a, sizeof(a));
	return sizeof(a);
}
declare_trace_evt(sig, PROTO(struct timeval tv), tv);


/* Enable / disable tracing */

extern const struct traced_event event_name(test1);
extern const struct traced_event event_name(test2);

void sigusr1_handler(int sig)
{
	struct timeval tv;

	event_enable(&event_name(sig));
	gettimeofday(&tv, NULL);
	sig_trace(tv);
	if (is_event_enabled(&event_name(test1)))
		event_disable(&event_name(test1));
	else
		event_enable(&event_name(test1));
	if (is_event_enabled(&event_name(test2)))
		event_disable(&event_name(test2));
	else
		event_enable(&event_name(test2));
}

int sys_init(void)
{
	signal(SIGUSR1, sigusr1_handler);
	return 0;
}

int sys_fork_loop(void (*l)(const char *), const char *n)
{
	pid_t p;

	p = fork();
	switch(p) {
	case -1:
		perror("fork");
		return p;
	case 0:
		/* Child */
		l(n);
		exit(0);
	default:
		/* Parent */
		return p;
	}
	/* NEVER REACHED */
	return -1;
}

int sys_sleep(int seconds)
{
	return sleep(seconds);
}

int sys_lock_tracing_buf(void)
{
	int stat;

	while((stat = sem_wait(&local_shared_area->bufsem)) == -1 &&
	      errno == -EINTR)
		continue;

	return stat;
}

int sys_unlock_tracing_buf(void)
{
	return sem_post(&local_shared_area->bufsem);
}
