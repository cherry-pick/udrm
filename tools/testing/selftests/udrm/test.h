#ifndef __TEST_H
#define __TEST_H

/*
 * Copyright (C) 2013-2016 Red Hat, Inc.
 *
 * udrm is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* include standard environment for all tests */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/udrm.h>
#include <linux/sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

enum {
	TEST_OK,
	TEST_FAIL,
	TEST_SKIP,
};

extern char *test_path;

struct test {
	const char *name;
	int (*main) (void);
};

int test_api(void);

static const struct test tests[] = {
	{ .name = "api", .main = test_api },
};

int c_sys_clone(unsigned long flags, void *child_stack);

#define c_align_to(_val, _to) (((_val) + (_to) - 1) & ~((_to) - 1))

#endif /* __TEST_H */
