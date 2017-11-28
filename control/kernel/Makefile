SHELL := /bin/bash

obj-m += controller.o

.PHONY: uname
uname:
	@./uname.sh

all: uname
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm uname.h
