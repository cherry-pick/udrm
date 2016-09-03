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
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include "udrm.h"

struct udrm_cdev {
	struct mutex lock;
};

static int udrm_cdev_fop_open(struct inode *inode, struct file *file)
{
	struct udrm_cdev *cdev;

	cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
	if (!cdev)
		return -ENOMEM;

	mutex_init(&cdev->lock);

	file->private_data = cdev;
	return 0;
}

static int udrm_cdev_fop_release(struct inode *inode, struct file *file)
{
	struct udrm_cdev *cdev = file->private_data;

	mutex_destroy(&cdev->lock);
	kfree(cdev);

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

const struct file_operations udrm_cdev_fops = {
	.owner		= THIS_MODULE,
	.open		= udrm_cdev_fop_open,
	.release	= udrm_cdev_fop_release,
	.read		= udrm_cdev_fop_read,
	.poll		= udrm_cdev_fop_poll,
	.llseek		= no_llseek,
};
