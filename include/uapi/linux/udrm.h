#ifndef _UAPI_LINUX_UDRM_H
#define _UAPI_LINUX_UDRM_H

/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 */

#include <linux/ioctl.h>
#include <linux/types.h>

#define UDRM_IOCTL_MAGIC		0x99

struct udrm_cmd_plug {
	__u64 flags;
	__u64 n_edid;
	__u64 ptr_edid;
} __attribute__((__aligned__(8)));

enum {
	UDRM_CMD_REGISTER		= _IOWR(UDRM_IOCTL_MAGIC, 0x00,
					__u64),
	UDRM_CMD_UNREGISTER		= _IOWR(UDRM_IOCTL_MAGIC, 0x01,
					__u64),
	UDRM_CMD_PLUG			= _IOWR(UDRM_IOCTL_MAGIC, 0x02,
					struct udrm_cmd_plug),
	UDRM_CMD_UNPLUG			= _IOWR(UDRM_IOCTL_MAGIC, 0x03,
					__u64),
};

#endif /* _UAPI_LINUX_UDRM_H */
