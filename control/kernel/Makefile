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


.PHONY: lint
lint:
	find . -name "*.c"  -exec cppcheck --force {} \;
	find . -name "*.h"  -exec cppcheck --force {} \;

.PHONY: pretty
pretty:
	find . -name "*.c"  -exec indent -bsd {} \;
	find . -name "*.h"  -exec indent -bsd {} \;

.PHONY: trim
trim:
	find ./ -name "*.c" | xargs ttws.sh
	find ./ -name "*.h" | xargs ttws.sh
	find ./ -name "Makefile" | xargs ttws.sh
