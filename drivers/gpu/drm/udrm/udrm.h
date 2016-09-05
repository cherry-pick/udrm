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

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_gem.h>
#include <drm/drm_simple_kms_helper.h>
#include <linux/kernel.h>

struct miscdevice;
struct udrm_cdev;
struct udrm_device;

/* udrm devices */

struct udrm_device {
	unsigned long n_bindings;
	struct device dev;
	struct drm_device *ddev;
	struct rw_semaphore cdev_lock;
	struct udrm_cdev *cdev_unlocked;
	struct drm_simple_display_pipe pipe;
	struct drm_connector conn;
};

struct udrm_device *udrm_device_new(struct device *parent);
struct udrm_device *udrm_device_ref(struct udrm_device *udrm);
struct udrm_device *udrm_device_unref(struct udrm_device *udrm);
struct udrm_cdev *udrm_device_acquire(struct udrm_device *udrm);
struct udrm_cdev *udrm_device_release(struct udrm_device *udrm,
				      struct udrm_cdev *cdev);

bool udrm_device_is_new(struct udrm_device *udrm);
bool udrm_device_is_registered(struct udrm_device *udrm);
int udrm_device_register(struct udrm_device *udrm, struct udrm_cdev *cdev);
void udrm_device_unregister(struct udrm_device *udrm);

/* udrm gem */

struct udrm_bo {
	struct drm_gem_object base;
};

struct udrm_bo *udrm_bo_new(struct drm_device *ddev, size_t size);
void udrm_bo_free(struct drm_gem_object *dobj);

int udrm_dumb_create(struct drm_file *dfile,
		     struct drm_device *ddev,
		     struct drm_mode_create_dumb *args);
int udrm_dumb_map_offset(struct drm_file *dfile,
			 struct drm_device *ddev,
			 uint32_t handle,
			 uint64_t *offset);

/* udrm kms */

struct udrm_fb {
	struct drm_framebuffer base;
	struct udrm_bo *bo;
};

struct udrm_fb *udrm_fb_new(struct udrm_bo *bo,
			    const struct drm_mode_fb_cmd2 *cmd);

int udrm_kms_bind(struct udrm_device *udrm);
void udrm_kms_unbind(struct udrm_device *udrm);

/* udrm cdevs */

struct udrm_cdev {
	struct mutex lock;
	struct udrm_device *udrm;
	struct edid *edid;
};

extern struct miscdevice udrm_cdev_misc;

#endif /* __UDRM_UDRM_H */
