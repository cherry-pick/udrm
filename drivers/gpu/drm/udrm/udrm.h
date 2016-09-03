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
struct udrm_device;

/* udrm devices */

struct udrm_device *udrm_device_new(struct device *parent);
struct udrm_device *udrm_device_ref(struct udrm_device *udrm);
struct udrm_device *udrm_device_unref(struct udrm_device *udrm);
int udrm_device_register(struct udrm_device *udrm);
void udrm_device_unregister(struct udrm_device *udrm);

/* udrm cdevs */

extern struct miscdevice udrm_cdev_misc;

#endif /* __UDRM_UDRM_H */
