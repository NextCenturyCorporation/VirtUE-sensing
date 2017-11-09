obj-m += $@
control:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
