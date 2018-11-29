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
#ifndef __SYS_H__
#define __SYS_H__

/* Sys dependent, setup shared memory for tracing somehow */
extern int sys_init(void);

extern void *sys_setup_tracing_memory(unsigned int size);

extern int sys_fork_loop(void (*l)(const char *), const char *n);

extern void sys_sleep(int s);

#endif /* __SYS_H__ */

