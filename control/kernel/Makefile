SHELL := /bin/bash
MY_CCFLAGS += -std=gnu99 -fms-extensions -g -DDEBUG
ccflags-y += ${MY_CCFLAGS}
CC += ${MY_CCFLAGS}
obj-m += controller.o controller-interface.o

.PHONY: uname
uname:
	@./uname.sh

all: uname
		$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

debug: uname
		$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
		EXTRA_CFLAGS="${MY_CCFLAGS}"

clean:
		$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
		rm uname.h &>/dev/null


