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
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include "udrm.h"

static struct miscdevice udrm_misc = {
	.fops		= &udrm_cdev_fops,
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= KBUILD_MODNAME,
};

static int __init udrm_init(void)
{
	int r;

	r = misc_register(&udrm_misc);
	if (r < 0)
		return r;

	pr_info("loaded\n");
	return 0;
}

static void __exit udrm_exit(void)
{
	misc_deregister(&udrm_misc);
	pr_info("unloaded\n");
}

module_init(udrm_init);
module_exit(udrm_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual DRM Device Driver");
MODULE_ALIAS("devname:" KBUILD_MODNAME);
