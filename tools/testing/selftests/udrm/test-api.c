/*
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include "test.h"

/* make sure /dev/udrm exists, is a cdev and accessible */
static void test_api_cdev(void)
{
	struct stat st;
	int r, fd;

	r = access(test_path, F_OK);
	assert(r >= 0);

	r = stat(test_path, &st);
	assert(r >= 0);
	assert((st.st_mode & S_IFMT) == S_IFCHR);

	fd = open(test_path, O_RDWR | O_CLOEXEC | O_NONBLOCK | O_NOCTTY);
	assert(fd >= 0);

	close(fd);
}

/* make sure simple REGISTER/UNREGISTER works */
static void test_api_registration(void)
{
	int r, fd;

	fd = open(test_path, O_RDWR | O_CLOEXEC | O_NONBLOCK | O_NOCTTY);
	assert(fd >= 0);

	r = ioctl(fd, UDRM_CMD_UNREGISTER, NULL);
	assert(r < 0 && errno == ENOTCONN);

	r = ioctl(fd, UDRM_CMD_REGISTER, NULL);
	assert(r >= 0);

	r = ioctl(fd, UDRM_CMD_REGISTER, NULL);
	assert(r < 0 && errno == EISCONN);

	r = ioctl(fd, UDRM_CMD_UNREGISTER, NULL);
	assert(r >= 0);

	r = ioctl(fd, UDRM_CMD_UNREGISTER, NULL);
	assert(r < 0 && errno == ESHUTDOWN);

	r = ioctl(fd, UDRM_CMD_REGISTER, NULL);
	assert(r < 0 && errno == ESHUTDOWN);

	close(fd);
}

int test_api(void)
{
	test_api_cdev();
	test_api_registration();

	return TEST_OK;
}
