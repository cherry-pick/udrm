/*
 * Copyright (C) 2015-2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <uapi/linux/udrm.h>
#include "udrm.h"

struct udrm_cdev {
	struct mutex lock;
	struct udrm_device *udrm;
};

static struct udrm_cdev *udrm_cdev_free(struct udrm_cdev *cdev)
{
	if (cdev) {
		udrm_device_unref(cdev->udrm);
		mutex_destroy(&cdev->lock);
		kfree(cdev);
	}

	return NULL;
}

static struct udrm_cdev *udrm_cdev_new(void)
{
	struct udrm_cdev *cdev;
	int r;

	cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
	if (!cdev)
		return ERR_PTR(-ENOMEM);

	mutex_init(&cdev->lock);

	cdev->udrm = udrm_device_new(udrm_cdev_misc.this_device);
	if (IS_ERR(cdev->udrm)) {
		r = PTR_ERR(cdev->udrm);
		cdev->udrm = NULL;
		goto error;
	}

	return cdev;

error:
	udrm_cdev_free(cdev);
	return ERR_PTR(r);
}

static int udrm_cdev_fop_open(struct inode *inode, struct file *file)
{
	struct udrm_cdev *cdev;

	cdev = udrm_cdev_new();
	if (IS_ERR(cdev))
		return PTR_ERR(cdev);

	file->private_data = cdev;
	return 0;
}

static int udrm_cdev_fop_release(struct inode *inode, struct file *file)
{
	struct udrm_cdev *cdev = file->private_data;

	udrm_device_unregister(cdev->udrm);
	udrm_cdev_free(cdev);

	return 0;
}

static ssize_t udrm_cdev_fop_read(struct file *file,
				  char __user *data,
				  size_t n_data,
				  loff_t *offset)
{
	return -EAGAIN;
}

static unsigned int udrm_cdev_fop_poll(struct file *file, poll_table *wait)
{
	return 0;
}

static int udrm_cdev_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EIO;
}

static long udrm_cdev_fop_ioctl(struct file *file,
				unsigned int cmd,
				unsigned long arg)
{
	struct udrm_cdev *cdev = file->private_data;
	int r = 0;

	mutex_lock(&cdev->lock);
	switch (cmd) {
	case UDRM_CMD_REGISTER:
		if (unlikely(arg))
			r = -EINVAL;
		else if (udrm_device_is_registered(cdev->udrm))
			r = -EISCONN;
		else if (!udrm_device_is_new(cdev->udrm))
			r = -ESHUTDOWN;
		else
			r = udrm_device_register(cdev->udrm, cdev);
		break;
	case UDRM_CMD_UNREGISTER:
		if (unlikely(arg))
			r = -EINVAL;
		else if (udrm_device_is_new(cdev->udrm))
			r = -ENOTCONN;
		else if (!udrm_device_is_registered(cdev->udrm))
			r = -ESHUTDOWN;
		else
			udrm_device_unregister(cdev->udrm);
		break;
	case UDRM_CMD_PLUG:
		r = -ENOTTY; /* XXX: not implemented */
		break;
	case UDRM_CMD_UNPLUG:
		r = -ENOTTY; /* XXX: not implemented */
		break;
	default:
		r = -ENOTTY;
		break;
	}
	mutex_unlock(&cdev->lock);

	return r;
}

static const struct file_operations udrm_cdev_fops = {
	.owner		= THIS_MODULE,
	.open		= udrm_cdev_fop_open,
	.release	= udrm_cdev_fop_release,
	.read		= udrm_cdev_fop_read,
	.poll		= udrm_cdev_fop_poll,
	.mmap		= udrm_cdev_fop_mmap,
	.unlocked_ioctl	= udrm_cdev_fop_ioctl,
	.compat_ioctl	= udrm_cdev_fop_ioctl,
	.llseek		= no_llseek,
};

struct miscdevice udrm_cdev_misc = {
	.fops		= &udrm_cdev_fops,
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= KBUILD_MODNAME,
};
