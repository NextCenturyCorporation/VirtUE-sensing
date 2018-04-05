SHELL := /bin/bash
MY_CCFLAGS += -std=gnu99 -fms-extensions
ccflags-y += ${MY_CCFLAGS}
obj-m += controller.o
controller-y := controller-common.o controller-interface.o lsof-probe.o jsmn/jsmn.o

.PHONY: uname
uname:
	@./uname.sh

.PHONY: all
all: clean lint test-module

test-module : ccflags-y +=  -g -DDEBUG -fno-inline
test-module : uname
	@echo ${ccflags-y}
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
		$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
		rm uname.h &>/dev/null  ||:

module : ccflags-y +=  -02 -f-inline
module : uname
	@echo ${ccflags-y}
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

.PHONY: jsonl-clean
jsonl-clean:
	rm jsonl jsonl.o jsmn.o &>/dev/null ||:

jsonl: jsonl.c jsmn/jsmn.c
	  gcc -D JSMN_PARENT_LINKS -D JSMN_STRICT -D USERSPACE -g -c jsonl.c jsmn/jsmn.c
		gcc -g -o jsonl jsonl.o jsmn.o

.PHONY: jsonl-test
jsonl-test: jsonl
	./jsonl --verbose --in-file test.jsonl

.PHONY: jsonl-debug
jsonl-debug: jsonl-clean jsonl
	gdb jsonl -x jsonl.gdb

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
