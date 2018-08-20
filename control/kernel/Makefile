SHELL := /bin/bash
MY_CCFLAGS += -std=gnu99 -fms-extensions
ccflags-y += ${MY_CCFLAGS}
OBJECT_FILES_NON_STANDARD := y
obj-m += controller.o
controller-y := controller-common.o controller-interface.o lsof-sensor.o \
	ps-sensor.o jsmn/jsmn.o sysfs-sensor.o jsonl-parse.o
OBJECT_FILES_NON_STANDARD_controller-common.o := n

.PHONY: default
default: rebuild ;

.PHONY: uname
uname:
	@./uname.sh

.PHONY: import-header
import-header:
	@sudo python import-kernel-symbols.py import-header.h.in /boot/System.map-`uname -r` > import-header.h

.PHONY: all
all: clean lint test-module

.PHONY: rebuild
rebuild: clean test-module disassemble

test-module : ccflags-y +=  -g -DDEBUG -fno-inline
test-module : OBJECT_FILES_NON_STANDARD := y
test-module : uname
test-module : import-header
	@echo ${ccflags-y}
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
		$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
		rm uname.h &>/dev/null  ||:
		rm import-header.h &>/dev/null  ||:
		rm *.o.asm &> /dev/null ||:

module : ccflags-y +=  -02 -f-inline
module : uname
module : import-header
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
	find ./ -name "*.c" | xargs ./ttws.sh
	find ./ -name "*.h" | xargs ./ttws.sh
	find ./ -name "*.py" | xargs ./ttws.sh
	find ./ -name "Makefile" | xargs ./ttws.sh

.PHONY: disassemble
disassemble:
	find ./ -name "*.o" | xargs ./dis.sh
