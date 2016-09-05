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
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "udrm.h"

struct udrm_bo *udrm_bo_new(struct drm_device *ddev, size_t size)
{
	struct udrm_bo *bo;
	int r;

	WARN_ON(!size || (size & ~PAGE_MASK) != 0);

	bo = kzalloc(sizeof(*bo), GFP_KERNEL);
	if (!bo)
		return ERR_PTR(-ENOMEM);

	r = drm_gem_object_init(ddev, &bo->base, size);
	if (r < 0) {
		kfree(bo);
		return ERR_PTR(r);
	}

	return bo;
}

void udrm_bo_free(struct drm_gem_object *dobj)
{
	struct udrm_bo *bo = container_of(dobj, struct udrm_bo, base);

	drm_gem_object_release(dobj);
	kfree(bo);
}

int udrm_dumb_create(struct drm_file *dfile,
		     struct drm_device *ddev,
		     struct drm_mode_create_dumb *args)
{
	struct udrm_bo *bo;
	int r;

	/* overflow checks are done by DRM core */
	args->pitch = DIV_ROUND_UP(args->bpp, 8) * args->width;
	args->size = PAGE_ALIGN(args->pitch * args->height);

	bo = udrm_bo_new(ddev, args->size);
	if (IS_ERR(bo))
		return PTR_ERR(bo);

	r = drm_gem_handle_create(dfile, &bo->base, &args->handle);
	drm_gem_object_unreference_unlocked(&bo->base);
	return r;
}

int udrm_dumb_map_offset(struct drm_file *dfile,
			 struct drm_device *ddev,
			 uint32_t handle,
			 uint64_t *offset)
{
	struct drm_gem_object *dobj;
	int r;

	dobj = drm_gem_object_lookup(dfile, handle);
	if (!dobj)
		return -ENOENT;

	r = drm_gem_create_mmap_offset(dobj);
	if (r >= 0)
		*offset = drm_vma_node_offset_addr(&dobj->vma_node);
	drm_gem_object_unreference_unlocked(dobj);
	return r;
}
