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
#include <drm/drm_gem.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include "udrm.h"

static struct drm_driver udrm_drm_driver;
static DEFINE_MUTEX(udrm_drm_lock);

static void udrm_device_free(struct device *dev)
{
	struct udrm_device *udrm = container_of(dev, struct udrm_device, dev);

	WARN_ON(udrm->cdev_unlocked);
	drm_dev_unref(udrm->ddev);
	WARN_ON(udrm->n_bindings > 0);
	kfree(udrm);
}

struct udrm_device *udrm_device_new(struct device *parent)
{
	static atomic64_t id_counter;
	struct udrm_device *udrm;
	int r;

	udrm = kzalloc(sizeof(*udrm), GFP_KERNEL);
	if (!udrm)
		return ERR_PTR(-ENOMEM);

	device_initialize(&udrm->dev);
	udrm->dev.release = udrm_device_free;
	udrm->dev.parent = parent;
	init_rwsem(&udrm->cdev_lock);

	r = dev_set_name(&udrm->dev, KBUILD_MODNAME "-%llu",
			 (unsigned long long)atomic64_inc_return(&id_counter));
	if (r < 0)
		goto error;

	udrm->ddev = drm_dev_alloc(&udrm_drm_driver, &udrm->dev);
	if (!udrm->ddev) {
		r = -ENOMEM;
		goto error;
	}

	udrm->ddev->dev_private = udrm;
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

struct udrm_cdev *udrm_device_acquire(struct udrm_device *udrm)
{
	if (udrm) {
		down_read(&udrm->cdev_lock);
		if (udrm->cdev_unlocked)
			return udrm->cdev_unlocked;
		up_read(&udrm->cdev_lock);
	}

	return NULL;
}

struct udrm_cdev *udrm_device_release(struct udrm_device *udrm,
				      struct udrm_cdev *cdev)
{
	if (!cdev || WARN_ON(!udrm))
		return NULL;

	WARN_ON(cdev != udrm->cdev_unlocked);
	up_read(&udrm->cdev_lock);
	return NULL;
}

static void udrm_device_unbind(struct udrm_device *udrm)
{
	lockdep_assert_held(&udrm_drm_lock);

	if (!udrm || WARN_ON(udrm->n_bindings < 1) || --udrm->n_bindings > 0)
		return;

	udrm_kms_unbind(udrm);
	udrm_device_unref(udrm);
}

static int udrm_device_bind(struct udrm_device *udrm)
{
	int r;

	lockdep_assert_held(&udrm_drm_lock);

	if (udrm->n_bindings < 1) {
		r = udrm_kms_bind(udrm);
		if (r < 0)
			return r;

		udrm_device_ref(udrm);
	}

	++udrm->n_bindings;
	return 0;
}

int udrm_device_register(struct udrm_device *udrm, struct udrm_cdev *cdev)
{
	int r;

	if (device_is_registered(&udrm->dev))
		return -EALREADY;

	mutex_lock(&udrm_drm_lock);

	down_write(&udrm->cdev_lock);
	udrm->cdev_unlocked = cdev;
	up_write(&udrm->cdev_lock);

	r = udrm_device_bind(udrm);
	if (r < 0)
		goto exit_cleanup;

	r = device_add(&udrm->dev);
	if (r < 0)
		goto exit_unbind;

	r = drm_dev_register(udrm->ddev, 0);
	if (r < 0)
		goto exit_del;

	r = 0;
	goto exit;

exit_del:
	device_del(&udrm->dev);
exit_unbind:
	udrm_device_unbind(udrm);
exit_cleanup:
	down_write(&udrm->cdev_lock);
	udrm->cdev_unlocked = NULL;
	up_write(&udrm->cdev_lock);
exit:
	mutex_unlock(&udrm_drm_lock);
	return r;
}

void udrm_device_unregister(struct udrm_device *udrm)
{
	mutex_lock(&udrm_drm_lock);
	if (udrm && device_is_registered(&udrm->dev)) {
		down_write(&udrm->cdev_lock);
		udrm->cdev_unlocked = NULL;
		up_write(&udrm->cdev_lock);

		drm_dev_unregister(udrm->ddev);
		device_del(&udrm->dev);
		udrm_device_unbind(udrm);
	}
	mutex_unlock(&udrm_drm_lock);
}

static int udrm_drm_fop_open(struct inode *inode, struct file *file)
{
	struct udrm_device *udrm;
	struct drm_device *ddev;
	int r;

	mutex_lock(&udrm_drm_lock);

	r = drm_open(inode, file);
	if (r < 0)
		goto exit;

	ddev = file->private_data;
	udrm = ddev->dev_private;

	r = udrm_device_bind(udrm);
	if (r < 0) {
		drm_release(inode, file);
		goto exit;
	}

	r = 0;

exit:
	mutex_unlock(&udrm_drm_lock);
	return r;
}

static int udrm_drm_fop_release(struct inode *inode, struct file *file)
{
	struct drm_device *ddev = file->private_data;
	struct udrm_device *udrm = ddev->dev_private;

	mutex_lock(&udrm_drm_lock);
	drm_release(inode, file);
	udrm_device_unbind(udrm);
	mutex_unlock(&udrm_drm_lock);

	return 0;
}

static const struct file_operations udrm_drm_fops = {
	.owner			= THIS_MODULE,
	.llseek			= noop_llseek,
	.open			= udrm_drm_fop_open,
	.release		= udrm_drm_fop_release,
};

static struct drm_driver udrm_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &udrm_drm_fops,
	.gem_free_object = udrm_bo_free,
	.dumb_create = udrm_dumb_create,
	.dumb_map_offset = udrm_dumb_map_offset,
	.dumb_destroy = drm_gem_dumb_destroy,
	.name = "udrm",
	.desc = "Virtual DRM Device Driver",
	.date = "20160903",
};
