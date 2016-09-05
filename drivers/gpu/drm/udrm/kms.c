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
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "udrm.h"

/* XXX: should be provided by hw */
static const uint32_t udrm_formats[] = {
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB8888,
};

static int udrm_conn_get_modes(struct drm_connector *conn)
{
	struct udrm_device *udrm = conn->dev->dev_private;
	struct drm_display_mode *mode;

	/* XXX: should be provided by hw */
	mode = drm_cvt_mode(udrm->ddev, 800, 600,
			    60, false, false, false);
	if (!mode)
		return 0;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(conn, mode);

	return 1;
}

static const struct drm_connector_helper_funcs udrm_conn_hops = {
	.get_modes	= udrm_conn_get_modes,
	.best_encoder	= drm_atomic_helper_best_encoder,
};

static enum drm_connector_status udrm_conn_detect(struct drm_connector *conn,
						  bool force)
{
	/* XXX: check hw state */
	return connector_status_connected;
}

static const struct drm_connector_funcs udrm_conn_ops = {
	.dpms			= drm_atomic_helper_connector_dpms,
	.reset			= drm_atomic_helper_connector_reset,
	.detect			= udrm_conn_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

void udrm_display_pipe_update(struct drm_simple_display_pipe *pipe,
			      struct drm_plane_state *plane_state)
{
	struct udrm_device *udrm = container_of(pipe, struct udrm_device, pipe);

	/* XXX: forward to hw */
	pipe->plane.fb = pipe->plane.state->fb;

	if (pipe->crtc.state && pipe->crtc.state->event) {
		spin_lock_irq(&udrm->ddev->event_lock);
		drm_crtc_send_vblank_event(&pipe->crtc,
					   pipe->crtc.state->event);
		spin_unlock_irq(&udrm->ddev->event_lock);
		pipe->crtc.state->event = NULL;
	}
}

static void udrm_display_pipe_enable(struct drm_simple_display_pipe *pipe,
				     struct drm_crtc_state *crtc_state)
{
	/* XXX: forward to hw */
}

static void udrm_display_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	/* XXX: forward to hw */
}

static const struct drm_simple_display_pipe_funcs udrm_pipe_ops = {
	.update		= udrm_display_pipe_update,
	.enable		= udrm_display_pipe_enable,
	.disable	= udrm_display_pipe_disable,
};

static int udrm_fb_create_handle(struct drm_framebuffer *dfb,
				 struct drm_file *dfile,
				 unsigned int *handle)
{
	struct udrm_fb *fb = container_of(dfb, struct udrm_fb, base);

	return drm_gem_handle_create(dfile, &fb->bo->base, handle);
}

static int udrm_fb_dirty(struct drm_framebuffer *dfb,
			 struct drm_file *dfile,
			 unsigned int flags,
			 unsigned int color,
			 struct drm_clip_rect *clips,
			 unsigned int n_clips)
{
	struct udrm_device *udrm = dfb->dev->dev_private;
	struct udrm_cdev *cdev;

	cdev = udrm_device_acquire(udrm);
	if (cdev) {
		/* XXX: report clips */
		udrm_device_release(udrm, cdev);
	}

	return 0;
}

static void udrm_fb_destroy(struct drm_framebuffer *dfb)
{
	struct udrm_fb *fb = container_of(dfb, struct udrm_fb, base);

	drm_framebuffer_cleanup(dfb);
	drm_gem_object_unreference_unlocked(&fb->bo->base);
	kfree(fb);
}

static const struct drm_framebuffer_funcs udrm_fb_ops = {
	.create_handle		= udrm_fb_create_handle,
	.dirty			= udrm_fb_dirty,
	.destroy		= udrm_fb_destroy,
};

struct udrm_fb *udrm_fb_new(struct udrm_bo *bo,
			    const struct drm_mode_fb_cmd2 *cmd)
{
	struct udrm_fb *fb;
	int r;

	if (cmd->flags)
		return ERR_PTR(-EINVAL);

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb)
		return ERR_CAST(fb);

	drm_gem_object_reference(&bo->base);
	fb->bo = bo;
	drm_helper_mode_fill_fb_struct(&fb->base, cmd);

	r = drm_framebuffer_init(bo->base.dev, &fb->base, &udrm_fb_ops);
	if (r < 0)
		goto error;

	return fb;

error:
	drm_gem_object_unreference_unlocked(&bo->base);
	kfree(fb);
	return ERR_PTR(r);
}

static struct drm_framebuffer *udrm_fb_create(struct drm_device *ddev,
					      struct drm_file *dfile,
					      const struct drm_mode_fb_cmd2 *c)
{
	struct drm_gem_object *dobj;
	struct udrm_fb *fb;

	dobj = drm_gem_object_lookup(dfile, c->handles[0]);
	if (!dobj)
		return ERR_PTR(-EINVAL);

	fb = udrm_fb_new(container_of(dobj, struct udrm_bo, base), c);
	drm_gem_object_unreference_unlocked(dobj);
	return IS_ERR(fb) ? ERR_CAST(fb) : &fb->base;
}

static const struct drm_mode_config_funcs udrm_kms_ops = {
	.fb_create		= udrm_fb_create,
	.atomic_check		= drm_atomic_helper_check,
	.atomic_commit		= drm_atomic_helper_commit,
};

int udrm_kms_bind(struct udrm_device *udrm)
{
	struct drm_connector *conn = &udrm->conn;
	struct drm_device *ddev = udrm->ddev;
	int r;

	if (WARN_ON(ddev->mode_config.funcs))
		return -ENOTRECOVERABLE;

	/* XXX: should be provided by hw */
	drm_mode_config_init(ddev);
	ddev->mode_config.min_width = 128;
	ddev->mode_config.max_width = 4096;
	ddev->mode_config.min_height = 128;
	ddev->mode_config.max_height = 4096;
	ddev->mode_config.preferred_depth = 24;
	ddev->mode_config.funcs = &udrm_kms_ops;
	drm_connector_helper_add(conn, &udrm_conn_hops);

	/* XXX: should be provided by hw */
	r = drm_mode_create_dirty_info_property(ddev);
	if (r < 0)
		goto error;

	r = drm_connector_init(ddev, conn, &udrm_conn_ops,
			       DRM_MODE_CONNECTOR_VIRTUAL);
	if (r < 0)
		goto error;

	/* XXX: should be provided by hw */
	drm_object_attach_property(&conn->base,
				   ddev->mode_config.dirty_info_property, 1);

	r = drm_simple_display_pipe_init(ddev, &udrm->pipe, &udrm_pipe_ops,
					 udrm_formats, ARRAY_SIZE(udrm_formats),
					 conn);
	if (r < 0)
		goto error;

	drm_mode_config_reset(ddev);
	return 0;

error:
	drm_mode_config_cleanup(ddev);
	return r;
}

void udrm_kms_unbind(struct udrm_device *udrm)
{
	if (udrm->ddev->mode_config.funcs)
		drm_mode_config_cleanup(udrm->ddev);
}
