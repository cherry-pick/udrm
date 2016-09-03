#
# This file is part of udrm. See COPYING for details.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#

all: module

module:
	$(MAKE) -f modules.mk \
		DIR=drivers/gpu/drm/udrm \
		CONFIG_DRM_UDRM=m

.PHONY: all module
