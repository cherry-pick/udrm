#
# Out-of-tree Kernel Modules
# This builds out-of-tree kernel modules. It must be invoked via the kbuild
# system and will build the .ko binaries. Anything below ./$DIR/ can be diffed
# against the upstream kernel tree to get patches that apply to both, upstream
# and this downstream out-of-tree repository.
# This will invoke the kbuild tools on $(DIR), building everything underneath
# that directory. Note that you very likely need to enable your configuration
# via CONFIG_FOOBAR=m via the environment.
#

DIR			?= drivers
KERNELVER		?= $(shell uname -r)
KERNELDIR 		?= /lib/modules/$(KERNELVER)/build
PWD			:= $(shell pwd)

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)/$(DIR)

.PHONY: module
