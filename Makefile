#
# This file is part of udrm. See COPYING for details.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#

all: module
.PHONY: all

module:
	$(MAKE) -f modules.mk \
		DIR=drivers/gpu/drm/udrm \
		CONFIG_DRM_UDRM=m
.PHONY: module

tests:
	CFLAGS="-g -O0" $(MAKE) -C tools/testing/selftests/udrm/
.PHONY: tests

tt-prepare: module
	-sudo sh -c 'dmesg -c > /dev/null'
	-sudo sh -c 'rmmod udrm$(UDRMEXT)'
	sudo sh -c 'insmod drivers/gpu/drm/udrm/udrm$(UDRMEXT).ko'
.PHONY: tt-prepare

stt: tests tt-prepare
	sudo tools/testing/selftests/udrm/udrm-test --module udrm$(UDRMEXT) ; (R=$$? ; dmesg ; exit $$R)
.PHONY: tt
