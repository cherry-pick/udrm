/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <drm/drmP.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "udrm.h"

struct udrm_device {
	struct device dev;
	struct drm_device *ddev;
};

static struct drm_driver udrm_drm_driver;
static DEFINE_MUTEX(udrm_drm_lock);

static void udrm_device_free(struct device *dev)
{
	struct udrm_device *udrm = container_of(dev, struct udrm_device, dev);

	drm_dev_unref(udrm->ddev);
	kfree(udrm);
}

struct udrm_device *udrm_device_new(struct device *parent)
{
	struct udrm_device *udrm;
	int r;

	udrm = kzalloc(sizeof(*udrm), GFP_KERNEL);
	if (!udrm)
		return ERR_PTR(-ENOMEM);

	device_initialize(&udrm->dev);
	udrm->dev.release = udrm_device_free;
	udrm->dev.parent = parent;

	udrm->ddev = drm_dev_alloc(&udrm_drm_driver, &udrm->dev);
	if (!udrm->ddev) {
		r = -ENOMEM;
		goto error;
	}

	return udrm;

error:
	put_device(&udrm->dev);
	return ERR_PTR(r);
}

struct udrm_device *udrm_device_ref(struct udrm_device *udrm)
{
	if (udrm)
		get_device(&udrm->dev);

	return udrm;
}

struct udrm_device *udrm_device_unref(struct udrm_device *udrm)
{
	if (udrm)
		put_device(&udrm->dev);

	return NULL;
}

static void udrm_device_unbind(struct udrm_device *udrm)
{
}

static int udrm_device_bind(struct udrm_device *udrm)
{
	return 0;
}

int udrm_device_register(struct udrm_device *udrm)
{
	static u64 id_counter;
	int r;

	mutex_lock(&udrm_drm_lock);

	if (WARN_ON(!udrm || device_is_registered(&udrm->dev))) {
		r = -ENOTRECOVERABLE;
		goto exit;
	}

	r = udrm_device_bind(udrm);
	if (r < 0)
		goto exit;

	r = dev_set_name(&udrm->dev, KBUILD_MODNAME "-%llu",
			 (unsigned long long)id_counter);
	if (r < 0)
		goto exit;

	r = device_add(&udrm->dev);
	if (r < 0)
		goto exit;

	r = drm_dev_register(udrm->ddev, 0);
	if (r < 0)
		goto exit;

	++id_counter;
	r = 0;

exit:
	if (r < 0) {
		if (device_is_registered(&udrm->dev))
			device_del(&udrm->dev);
		udrm_device_unbind(udrm);
	}
	mutex_unlock(&udrm_drm_lock);
	return r;
}

void udrm_device_unregister(struct udrm_device *udrm)
{
	mutex_lock(&udrm_drm_lock);

	if (udrm && device_is_registered(&udrm->dev)) {
		drm_dev_unregister(udrm->ddev);
		device_del(&udrm->dev);
	}

	mutex_unlock(&udrm_drm_lock);
}

static const struct file_operations udrm_drm_fops = {
	.owner = THIS_MODULE,
	.llseek = noop_llseek,
};

static struct drm_driver udrm_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &udrm_drm_fops,

	.name = "udrm",
	.desc = "Virtual DRM Device Driver",
	.date = "20160903",
};
