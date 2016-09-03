#ifndef __UDRM_UDRM_H
#define __UDRM_UDRM_H

/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>

struct miscdevice;

/* udrm cdevs */

extern struct miscdevice udrm_cdev_misc;

#endif /* __UDRM_UDRM_H */
